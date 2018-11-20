//
// Created by max on 11/20/18.
//

#include "SoftTimer.h"


void SoftTimer::setCount(unsigned long newCount) {
    count = newCount;
}

void SoftTimer::setAction(const SoftTimer::timerAction& newAction) {
    action = newAction;
}

void SoftTimer::tick() {
    if (count > 0) {
        count--;
        if (count == 0) {
            action();
        }
    }
}
