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
    std::unique_lock<std::mutex> uLock(_mtx);

    // Message queue stacks up and causes lesser used intersections to function incorrectly. 
    //  Need to clear queue because we only care about the current state, not the history of states.

    // while (_queue.size() > 0)
    // {
    //     _queue.pop_front();    
    // }
    _queue.clear();
    _cond.wait(uLock, [this]{return !_queue.empty();});

    T message = std::move(_queue.front());
    
    _queue.pop_front();

    return message;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    
    // Only receiving green messages
    std::lock_guard<std::mutex> uLock(_mtx);
    _queue.push_back(std::move(msg));
    _cond.notify_one();
}


/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight()
{
    std::lock_guard<std::mutex> lck(_mutex);
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    TrafficLightPhase currentPhase;
    while (true)
    {
        currentPhase = _messageQueue.receive();
        if (currentPhase == TrafficLightPhase::green) break;
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    std::lock_guard<std::mutex> lck(_mutex);
    return _currentPhase;
}

void TrafficLight::setCurrentPhase(TrafficLightPhase phase)
{
    std::lock_guard<std::mutex> lck(_mutex);
    this->_currentPhase = phase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. 
        // To do this, use the thread queue in the base class. 
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

    // I followed the structure of the infinite loop in "class Vehicle"

    // init Stopwatch
    auto lastUpdate = std::chrono::system_clock::now();
    float cycleTime = 4 + 2 * (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)); 

    while (true)
    {
        // Pause to lower CPU Utilization
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        long timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - lastUpdate).count(); 

        if (timeSinceLastUpdate > cycleTime)
        {
            // cycleTime has elapsed, so time to change phases
            TrafficLightPhase currentPhase = this->getCurrentPhase();
            if (currentPhase == TrafficLightPhase::red) 
            {
                currentPhase = TrafficLightPhase::green;
            } 
            else if (currentPhase == TrafficLightPhase::green) 
            {
                currentPhase = TrafficLightPhase::red;
            }

            this->setCurrentPhase(currentPhase);

            // Send Update message to queue
            // only need green message because the only user of the message queue looks for green lights and ignores red lights.
            if (currentPhase == TrafficLightPhase::green) _messageQueue.send(std::move(currentPhase));

            // Restart StopWatch
            lastUpdate = std::chrono::system_clock::now();

            // New Random cycle time between 4 and 6 seconds
            cycleTime = 4 + 2 * (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)); 
        }
    }
}
