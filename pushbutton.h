#include <Bounce2.h>
class PushButton : public Bounce {
    bool longpressActive=false;
    const unsigned long longPresInterval_=3000;
    public:
    enum press {none,longpress,shortpress,down,up};
    press update();
};