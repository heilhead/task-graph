#include <tasks.h>
#include "gtest/gtest.h"

using namespace std;

class TaskGraphTest : public ::testing::Test {
protected:
    virtual void SetUp() {
    };

    virtual void TearDown() {
    };

    virtual void verify(int index) {
        EXPECT_EQ(true, true);
    };
};

TEST_F(TaskGraphTest, TestAll) {
    verify(0);
}
