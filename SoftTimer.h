//
// Created by max on 11/20/18.
//

#ifndef METRONOME_SOFTTIMER_H
#define METRONOME_SOFTTIMER_H

/*
 * Software timer - allows longer timing delays by decrementing a variable
 * every time timer0 overflows
 */



class SoftTimer {
    typedef void (*timerAction)();
public:
    SoftTimer() noexcept : count(0), action(no_action) {}

    void setCount(unsigned long newCount);
    void setAction(const timerAction&);
    void tick();


private:
    unsigned long count;
    void (*action)();

    static void no_action() {};


};


#endif //METRONOME_SOFTTIMER_H
