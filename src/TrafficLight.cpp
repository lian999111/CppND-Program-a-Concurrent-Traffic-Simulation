#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 
    std::unique_lock<std::mutex> ulck(_mtx);    // it will be temporarily unlocked in wait()
    _cond.wait(ulck, [this] { return !_queue.empty(); });   // predicate lambda function to avoid spurious wake-ups

    // Using _queue.front() and _queue.pop_front() is not proper here, as _queue in an infrequently-used intersection will stack up.
    // That is, the front element may be out-dated and unrelevant. 
    // _queue.back() on the other hand returns the most recent message, which we care. Also, clear() should be called to flush out unrelevant history.
    // IMO, message passing of this project needs just a variable to store the latest signal instead of a queue to store the history.

    // Ref: https://knowledge.udacity.com/questions/98313
    T msg = _queue.back();
    _queue.clear();    
    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::lock_guard<std::mutex> lck(_mtx);
    _queue.push_back(std::move(msg));
    _cond.notify_one();
}


/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true) {
        TrafficLightPhase msg = _msgQueue.receive();
        if (msg == TrafficLightPhase::green) {
            return;
        }
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

    // Pick a random cycle duration between 4 and 6 seconds
    std::random_device rd;
    std::mt19937 engine(rd());
    std::uniform_real_distribution<> distr(4, 6);
    float cycleDuration = distr(engine);

    // Setup stop watch
    std::chrono::time_point<std::chrono::system_clock> lastTimestamp;
    lastTimestamp = std::chrono::system_clock::now();

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        float elapsedTime = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::system_clock::now() - lastTimestamp).count();
        if(elapsedTime >= cycleDuration) {
            // Toggle the light
            if (_currentPhase == TrafficLightPhase::red) {
                _currentPhase = TrafficLightPhase::green;
                // PF.4b : send the phase to the message queue
                // I think send(std::move(_currentPhase)) causes a confusion about what state _currentPhase is in after being moved from
                // So here I pass an rvalue enum instead of moving from _currentPhase
                _msgQueue.send(TrafficLightPhase::green);
                
            } else {
                _currentPhase = TrafficLightPhase::red;
                // PF.4b : send the phase to the message queue
                _msgQueue.send(TrafficLightPhase::red);
            }
            lastTimestamp = std::chrono::system_clock::now();
        }
    }
}

