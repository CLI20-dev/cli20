#include "template.hh"

#include <gtest/gtest.h>

TEST(TemplateTest, Hello) { EXPECT_EQ(cxx_template::hello(), "Hello, World!"); }
