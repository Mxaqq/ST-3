// Copyright 2021 GHA Test Team

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>
#include <thread>
#include <chrono>

#include "TimedDoor.h"

class MockDoor : public Door {

public:
    MOCK_METHOD(void, lock, (), (override));
    MOCK_METHOD(void, unlock, (), (override));
    MOCK_METHOD(bool, isDoorOpened, (), (override));
};

class MockTimerClient : public TimerClient {

public: 
    MOCK_METHOD(void, Timeout, (), (override));
};

TEST(DoorTimerAdapterTest, CallsThrowStateIfDoorOpened) {
    class TestDoor : public TimedDoor {

    public: 
        bool shouldThrow = true;
        TestDoor() : TimedDoor(1) {}
        bool isDoorOpened() override { return shouldThrow; }
        void throwState() override { throw std::runtime_error("BOOM"); }
    };

    TestDoor door;
    DoorTimerAdapter adapter(door);
    EXPECT_THROW(adapter.Timeout(), std::runtime_error);
}

TEST(DoorTimerAdapterTest, DoesNothingIfDoorClosed) {
    class TestDoor : public TimedDoor {

    public: 
        bool shouldThrow = false;
        TestDoor() : TimedDoor(1) {}
        bool isDoorOpened() override { return shouldThrow; }
        void throwState() override { FAIL() << "Should not throw"; }
    };

    TestDoor door;
    DoorTimerAdapter adapter(door);
    EXPECT_NO_THROW(adapter.Timeout());
}

TEST(TimerTest, CallsTimeoutAfterDelay) {
    class TestClient : public TimerClient {
    public: 

        bool called = false;
        void Timeout() override { called = true; }
    };

    TestClient client;
    Timer timer;
    timer.tregister(1, &client);

    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_TRUE(client.called);
}

class TimedDoorTest : public ::testing::Test {

protected: 
    TimedDoor* door;

    void SetUp() override {
        door = new TimedDoor(1);
    }

    void TearDown() override {
        delete door;
    }
};

TEST_F(TimedDoorTest, UnlockOpensDoor) {
    door->unlock();
    EXPECT_TRUE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, LockClosesDoor) {
    door->lock();
    EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, NoThrowIfClosedBeforeTimeout) {
    TimedDoor d(1);
    d.unlock();
    d.lock();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_NO_THROW({
        });
}

TEST(TimedDoorExtraTest, TimeoutIsSetCorrectly) {
    TimedDoor door(5);
    EXPECT_EQ(door.getTimeOut(), 5);
}

TEST(TimedDoorExtraTest, ThrowStateThrowsException) {
    TimedDoor door(1);
    EXPECT_THROW(door.throwState(), std::runtime_error);
}

TEST(TimedDoorExtraTest, MultipleUnlocksStillThrowOnce) {
    TimedDoor door(1);
    door.unlock();
    door.unlock();

    try {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        FAIL() << "Expected runtime_error not thrown";
    }
    catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "Door left open too long!");
    }
    catch (...) {
        FAIL() << "Expected runtime_error, got different exception";
    }
}

TEST(TimerExtraTest, TimeoutNotCalledImmediately) {
    class TestClient : public TimerClient {
    public: 

        bool called = false;
        void Timeout() override { called = true; }
    };

    TestClient client;
    Timer timer;
    timer.tregister(1, &client);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_FALSE(client.called);
}
