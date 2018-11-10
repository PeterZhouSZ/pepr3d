#ifdef _TEST_

#include <gtest/gtest.h>

#include "geometry/ColorManager.h"

TEST(ColorManager, constructor_basic) {
    pepr3d::ColorManager cm;
    EXPECT_EQ(cm.size(), 4);
    EXPECT_LE(cm.size(), PEPR3D_MAX_PALETTE_COLORS);
    EXPECT_EQ(cm.getColor(0).r, 1);
}

TEST(ColorManager, setColor) {
    pepr3d::ColorManager cm;
    const glm::vec4 red(1, 0, 0, 1);
    const glm::vec4 yellow(1, 1, 0, 1);
    EXPECT_EQ(cm.getColor(0), red);

    const auto color1 = cm.getColor(1);
    const auto color2 = cm.getColor(2);
    const auto color3 = cm.getColor(3);

    cm.setColor(0, yellow);
    EXPECT_EQ(cm.getColor(0), yellow);
    EXPECT_EQ(cm.size(), 4);
    EXPECT_EQ(cm.getColor(1), color1);
    EXPECT_EQ(cm.getColor(2), color2);
    EXPECT_EQ(cm.getColor(3), color3);
}

TEST(ColorManager, constructor_iterator) {
    std::vector<glm::vec4> newColors;
    newColors.reserve(PEPR3D_MAX_PALETTE_COLORS);
    for(int i = 0; i < PEPR3D_MAX_PALETTE_COLORS; ++i) {
        newColors.emplace_back(i / static_cast<float>(PEPR3D_MAX_PALETTE_COLORS), 0, 0, 1);
    }

    pepr3d::ColorManager cm(newColors.begin(), newColors.end());

    EXPECT_EQ(cm.size(), PEPR3D_MAX_PALETTE_COLORS);
    for(int i = 0; i < PEPR3D_MAX_PALETTE_COLORS; ++i) {
        EXPECT_EQ(cm.getColor(i), glm::vec4(i / static_cast<float>(PEPR3D_MAX_PALETTE_COLORS), 0, 0, 1));
    }

    EXPECT_EQ(newColors.size(), PEPR3D_MAX_PALETTE_COLORS);
}

TEST(ColorManager, constructor_iterator_above_limit) {
    std::vector<glm::vec4> newColors;
    newColors.reserve(PEPR3D_MAX_PALETTE_COLORS + 1);
    for(int i = 0; i < PEPR3D_MAX_PALETTE_COLORS + 1; ++i) {
        newColors.emplace_back(i / static_cast<float>(PEPR3D_MAX_PALETTE_COLORS + 1), 0, 0, 1);
    }

    pepr3d::ColorManager cm(newColors.begin(), newColors.end());

    EXPECT_EQ(cm.size(), PEPR3D_MAX_PALETTE_COLORS);
    for(int i = 0; i < PEPR3D_MAX_PALETTE_COLORS; ++i) {
        EXPECT_EQ(cm.getColor(i), glm::vec4(i / static_cast<float>(PEPR3D_MAX_PALETTE_COLORS + 1), 0, 0, 1));
    }

    EXPECT_EQ(newColors.size(), PEPR3D_MAX_PALETTE_COLORS + 1);
}

TEST(ColorManager, replaceColors_iterators) {
    std::vector<glm::vec4> newColors;
    newColors.reserve(PEPR3D_MAX_PALETTE_COLORS);
    for(int i = 0; i < PEPR3D_MAX_PALETTE_COLORS; ++i) {
        newColors.emplace_back(i / static_cast<float>(PEPR3D_MAX_PALETTE_COLORS), 0, 0, 1);
    }

    pepr3d::ColorManager cm;
    EXPECT_EQ(cm.size(), 4);

    cm.replaceColors(newColors.begin() + 1, newColors.end() - 1);

    EXPECT_EQ(cm.size(), PEPR3D_MAX_PALETTE_COLORS - 2);
    for(int i = 0; i < PEPR3D_MAX_PALETTE_COLORS - 2; ++i) {
        EXPECT_EQ(cm.getColor(i), glm::vec4((i + 1) / static_cast<float>(PEPR3D_MAX_PALETTE_COLORS), 0, 0, 1));
    }

    EXPECT_EQ(newColors.size(), PEPR3D_MAX_PALETTE_COLORS);
}

TEST(ColorManager, replaceColors_iterators_above_limit) {
    std::vector<glm::vec4> newColors;
    newColors.reserve(PEPR3D_MAX_PALETTE_COLORS + 3);
    for(int i = 0; i < PEPR3D_MAX_PALETTE_COLORS + 3; ++i) {
        newColors.emplace_back(i / static_cast<float>(PEPR3D_MAX_PALETTE_COLORS + 3), 0, 0, 1);
    }

    pepr3d::ColorManager cm;
    EXPECT_EQ(cm.size(), 4);

    cm.replaceColors(newColors.begin() + 1, newColors.end() - 1);

    EXPECT_EQ(cm.size(), PEPR3D_MAX_PALETTE_COLORS);
    for(int i = 0; i < PEPR3D_MAX_PALETTE_COLORS - 2; ++i) {
        EXPECT_EQ(cm.getColor(i), glm::vec4((i + 1) / static_cast<float>(PEPR3D_MAX_PALETTE_COLORS + 3), 0, 0, 1));
    }

    EXPECT_EQ(newColors.size(), PEPR3D_MAX_PALETTE_COLORS + 3);
}

TEST(ColorManager, replaceColors_move_semantics) {
    std::vector<glm::vec4> newColors;
    newColors.reserve(PEPR3D_MAX_PALETTE_COLORS);
    for(int i = 0; i < PEPR3D_MAX_PALETTE_COLORS; ++i) {
        newColors.emplace_back(i / static_cast<float>(PEPR3D_MAX_PALETTE_COLORS), 0, 0, 1);
    }

    pepr3d::ColorManager cm;
    EXPECT_EQ(cm.size(), 4);

    cm.replaceColors(std::move(newColors));

    EXPECT_EQ(cm.size(), PEPR3D_MAX_PALETTE_COLORS);
    for(int i = 0; i < PEPR3D_MAX_PALETTE_COLORS; ++i) {
        EXPECT_EQ(cm.getColor(i), glm::vec4(i / static_cast<float>(PEPR3D_MAX_PALETTE_COLORS), 0, 0, 1));
    }

    EXPECT_EQ(newColors.size(), 0);
}

TEST(ColorManager, replaceColors_move_semantics_above_limit) {
    std::vector<glm::vec4> newColors;
    newColors.reserve(PEPR3D_MAX_PALETTE_COLORS + 1);
    for(int i = 0; i < PEPR3D_MAX_PALETTE_COLORS + 1; ++i) {
        newColors.emplace_back(i / static_cast<float>(PEPR3D_MAX_PALETTE_COLORS + 1), 0, 0, 1);
    }

    pepr3d::ColorManager cm;
    EXPECT_EQ(cm.size(), 4);

    cm.replaceColors(std::move(newColors));

    EXPECT_EQ(cm.size(), PEPR3D_MAX_PALETTE_COLORS);
    for(int i = 0; i < PEPR3D_MAX_PALETTE_COLORS; ++i) {
        EXPECT_EQ(cm.getColor(i), glm::vec4(i / static_cast<float>(PEPR3D_MAX_PALETTE_COLORS + 1), 0, 0, 1));
    }

    EXPECT_EQ(newColors.size(), 0);
}

TEST(ColorManager, constructor_color_generation) {
    pepr3d::ColorManager cm(PEPR3D_MAX_PALETTE_COLORS);
    EXPECT_EQ(cm.size(), PEPR3D_MAX_PALETTE_COLORS);
}
#endif