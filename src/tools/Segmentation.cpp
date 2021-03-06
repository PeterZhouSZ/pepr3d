#include "tools/Segmentation.h"
#include <random>
#include <vector>
#include "commands/CmdPaintSingleColor.h"
#include "geometry/SdfValuesException.h"
#include "ui/MainApplication.h"

namespace pepr3d {

void Segmentation::drawToSidePane(SidePane& sidePane) {
    if(!mGeometryCorrect) {
        sidePane.drawText("Polyhedron not built, since the geometry was damaged. Tool disabled.");
        return;
    }
    const bool isSdfComputed = mApplication.getCurrentGeometry()->isSdfComputed();
    if(!isSdfComputed) {
        sidePane.drawText("Warning: This computation may take a long time to perform.");
        if(sidePane.drawButton("Compute SDF")) {
            mApplication.enqueueSlowOperation([this]() { safeComputeSdf(mApplication); }, []() {}, true);
        }
        sidePane.drawTooltipOnHover("Compute the shape diameter function of the model to enable the segmentation.");
    } else {
        if(sidePane.drawButton("Segment!")) {
            computeSegmentation();
        }
        sidePane.drawTooltipOnHover("Start segmentation on the model.");
        sidePane.drawFloatDragger("Robustness", mNumberOfClusters, 0.25f, 0.0f, 100.0f, "%.0f %%", 70.0f);
        sidePane.drawTooltipOnHover(
            "Higher values increase the computation time and might result in better region grouping. The default value "
            "should be good for most use cases.");
        sidePane.drawFloatDragger("Edge tolerance", mSmoothingLambda, 0.25f, 0.0f, 100.0f, "%.0f %%", 70.0f);
        sidePane.drawTooltipOnHover(
            "The higher the number, the more the segmentation will tolerate sharp edges and thus make less segments. "
            "If you have more segments than you wanted, increase this value. If you have less, decrease.");
    }

    sidePane.drawSeparator();

    ColorManager& colorManager = mApplication.getCurrentGeometry()->getColorManager();
    if(mPickState) {
        sidePane.drawText("Segmented into " + std::to_string(mNumberOfSegments) +
                          " segments. Assign a color from the palette to each segment.");

        sidePane.drawColorPalette();

        for(const auto& toPaint : mSegmentToTriangleIds) {
            std::string displayText = "Segment " + std::to_string(toPaint.first);

            glm::vec4 colorOfSegment(0, 0, 0, 1);
            if(mNewColors[toPaint.first] != std::numeric_limits<size_t>::max()) {
                colorOfSegment = colorManager.getColor(mNewColors[toPaint.first]);
            } else {
                colorOfSegment = mSegmentationColors[toPaint.first];
            }
            glm::vec3 hsvButtonColor = ci::rgbToHsv(static_cast<ci::ColorA>(colorOfSegment));
            hsvButtonColor.y = 0.75;  // reduce the saturation
            ci::ColorA borderColor = ci::hsvToRgb(hsvButtonColor);
            float thickness = 3.0f;
            if(mHoveredTriangleId) {
                auto find = mTriangleToSegmentMap.find(*mHoveredTriangleId);
                assert(find != mTriangleToSegmentMap.end());
                assert(find->first == *mHoveredTriangleId);
                if(find->second == toPaint.first) {
                    borderColor = ci::ColorA(1, 0, 0, 1);
                    displayText = "Currently hovered";
                    thickness = 5.0f;
                }
            }
            if(sidePane.drawColoredButton(displayText.c_str(), borderColor, thickness)) {
                mNewColors[toPaint.first] = colorManager.getActiveColorIndex();
                const glm::vec4 newColor = colorManager.getColor(colorManager.getActiveColorIndex());
                setSegmentColor(toPaint.first, newColor);
            }
            sidePane.drawTooltipOnHover(
                "Click to color this segment with the currently active color from the palette.");
        }

        if(sidePane.drawButton("Accept")) {
            // Find the maximum index of the color assignment
            size_t maxColorIndex = std::numeric_limits<size_t>::min();
            for(const auto& segment : mSegmentToTriangleIds) {
                const size_t activeColorAssigned = mNewColors[segment.first];
                if(activeColorAssigned > maxColorIndex) {
                    maxColorIndex = activeColorAssigned;
                }
            }

            // If the user assigned colors are valid (i.e. there aren't colors out of the palette size), apply.
            if(maxColorIndex < colorManager.size()) {
                for(const auto& segment : mSegmentToTriangleIds) {
                    const size_t activeColorAssigned = mNewColors[segment.first];
                    auto proxy = segment.second;
                    CommandManager<Geometry>* const commandManager = mApplication.getCommandManager();
                    commandManager->execute(
                        std::make_unique<CmdPaintSingleColor>(std::move(proxy), activeColorAssigned), false);
                }
                reset();
                CI_LOG_I("Segmentation applied.");
            } else {  // Else report the error to the user and continue.
                mApplication.pushDialog(Dialog(
                    DialogType::Error, "Please assign a color to all segments",
                    "The segmentation was not accepted. Please assign all segments a color from the palette first!"));
                CI_LOG_W("Please assign all segments a color from the palette first.");
            }
        }
        sidePane.drawTooltipOnHover("Apply the results to the model.");
        if(sidePane.drawButton("Cancel")) {
            cancel();
        }
        sidePane.drawTooltipOnHover("Revert the model back to the previous coloring.");

        sidePane.drawSeparator();
    }
}

void Segmentation::cancel() {
    if(mPickState) {
        reset();
        CI_LOG_I("Segmentation canceled.");
    }
}

void Segmentation::onToolDeselect(ModelView& modelView) {
    if(!mGeometryCorrect) {
        return;
    }
    cancel();
}

void Segmentation::onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event) {
    ci::Ray mLastRay = modelView.getRayFromWindowCoordinates(event.getPos());
    const auto geometry = mApplication.getCurrentGeometry();
    if(geometry == nullptr) {
        mHoveredTriangleId = {};
        return;
    }
    mHoveredTriangleId = safeIntersectMesh(mApplication, mLastRay);
}

void Segmentation::drawToModelView(ModelView& modelView) {
    if(mHoveredTriangleId && mPickState && mGeometryCorrect) {
        modelView.drawTriangleHighlight(*mHoveredTriangleId);
    }
}

void Segmentation::onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) {
    if(!event.isLeftDown()) {
        return;
    }
    if(!mHoveredTriangleId) {
        return;
    }
    if(!mPickState) {
        return;
    }
    if(!mGeometryCorrect) {
        return;
    }

    auto find = mTriangleToSegmentMap.find(*mHoveredTriangleId);
    assert(find != mTriangleToSegmentMap.end());
    assert(find->first == *mHoveredTriangleId);

    const size_t segId = find->second;
    assert(segId < mNumberOfSegments);
    assert(segId < mNewColors.size());

    ColorManager& colorManager = mApplication.getCurrentGeometry()->getColorManager();

    mNewColors[segId] = colorManager.getActiveColorIndex();
    const glm::vec4 newColor = colorManager.getColor(colorManager.getActiveColorIndex());
    setSegmentColor(segId, newColor);
}

void Segmentation::reset() {
    mApplication.getModelView().toggleMeshOverride(false);

    mNumberOfSegments = 0;
    mPickState = false;
    mSdfEnabled = nullptr;

    mNewColors.clear();
    mSegmentationColors.clear();
    mHoveredTriangleId = {};

    mSegmentToTriangleIds.clear();
    mTriangleToSegmentMap.clear();
}

void Segmentation::computeSegmentation() {
    cancel();

    Geometry* geometry = mApplication.getCurrentGeometry();
    assert(geometry);

    float smoothingLambda = mSmoothingLambda / 100.0f;
    smoothingLambda = std::min<float>(smoothingLambda, 1.0f);
    smoothingLambda = std::max<float>(smoothingLambda, 0.01f);

    int numberOfClusters = static_cast<int>(std::floor(mNumberOfClusters / 100.0f / (1.0f / 14.0f)) + 2.0f);
    numberOfClusters = std::min<int>(numberOfClusters, 15);
    numberOfClusters = std::min<int>(numberOfClusters, static_cast<int>(geometry->getTriangleCount()) - 2);
    numberOfClusters = std::max<int>(2, numberOfClusters);

    assert(0.0f < smoothingLambda && smoothingLambda <= 1.0f);
    assert(2 <= numberOfClusters && numberOfClusters <= geometry->getTriangleCount() && numberOfClusters <= 15);

    try {
        mNumberOfSegments =
            geometry->segmentation(numberOfClusters, smoothingLambda, mSegmentToTriangleIds, mTriangleToSegmentMap);
    } catch(std::exception& e) {
        const std::string errorCaption = "Error: Failed to compute the segmentation";
        const std::string errorDescription =
            "An internal error occured while computing the the segmentation. If the problem persists, try re-loading "
            "the mesh.\n\n"
            "Please report this bug to the developers. The full description of the problem is:\n";
        mApplication.pushDialog(Dialog(DialogType::Error, errorCaption, errorDescription + e.what(), "OK"));
        return;
    }
    if(mNumberOfSegments > 0) {
        mPickState = true;

        // If we this time segment into more segments than previously
        if(mNewColors.size() < mNumberOfSegments) {
            mNewColors.resize(mNumberOfSegments);
            for(size_t i = 0; i < mNumberOfSegments; ++i) {
                mNewColors[i] = std::numeric_limits<size_t>::max();
            }
        }

        assert(mNumberOfSegments != std::numeric_limits<size_t>::max());
        assert(mNumberOfSegments <= PEPR3D_MAX_PALETTE_COLORS);

        // Generate new colors
        pepr3d::ColorManager::generateColors(mNumberOfSegments, mSegmentationColors);
        assert(mSegmentationColors.size() == mNumberOfSegments);

        // Create an override color buffer based on the segmentation
        std::vector<glm::vec4> newOverrideBuffer;
        newOverrideBuffer.resize(geometry->getTriangleCount() * 3);
        for(const auto& toPaint : mSegmentToTriangleIds) {
            for(const auto& tri : toPaint.second) {
                newOverrideBuffer[3 * tri] = mSegmentationColors[toPaint.first];
                newOverrideBuffer[3 * tri + 1] = mSegmentationColors[toPaint.first];
                newOverrideBuffer[3 * tri + 2] = mSegmentationColors[toPaint.first];
            }
        }
        mApplication.getModelView().toggleMeshOverride(true);
        mApplication.getModelView().initOverrideFromBasicGeoemtry();
        mApplication.getModelView().getOverrideColorBuffer() = newOverrideBuffer;
    }
}

void Segmentation::setSegmentColor(const size_t segmentId, const glm::vec4 newColor) {
    std::vector<glm::vec4>& overrideBuffer = mApplication.getModelView().getOverrideColorBuffer();
    auto segmentTris = mSegmentToTriangleIds.find(segmentId);

    if(segmentTris == mSegmentToTriangleIds.end()) {
        assert(false);
        return;
    }
    assert((*segmentTris).first == segmentId);

    assert(!overrideBuffer.empty());
    for(const auto& tri : (*segmentTris).second) {
        overrideBuffer[3 * tri] = newColor;
        overrideBuffer[3 * tri + 1] = newColor;
        overrideBuffer[3 * tri + 2] = newColor;
    }
}

void Segmentation::onNewGeometryLoaded(ModelView& modelView) {
    Geometry* const currentGeometry = mApplication.getCurrentGeometry();
    assert(currentGeometry != nullptr);
    if(currentGeometry == nullptr) {
        return;
    }

    mGeometryCorrect = currentGeometry->polyhedronValid();
    if(!mGeometryCorrect) {
        return;
    }
    CI_LOG_I("Model changed, segmentation reset");
    reset();
    mSdfEnabled = currentGeometry->sdfValuesValid();
}
}  // namespace pepr3d