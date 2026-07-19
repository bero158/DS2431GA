class cpuTimer {
		unsigned long _start = 0; //when and if the timer was started
		const unsigned long _limit; //implicit deadline (from constructor)
		const bool _restart; //restart after Overtime reached
	public:
		/*limit in ms*/
		cpuTimer(unsigned long limit=0,bool instantStart=false,bool restart=true) : _limit(limit), _restart(restart) { if (instantStart) start(); }
		void start(unsigned long start = millis()) { _start = start; }
		unsigned long getLimit() { return _limit; }
		bool isOverTime(unsigned long limit = 0) {
			bool over = !_start || ((millis() - _start) > ((limit) ? limit : getLimit())); 
			if (over && _restart) start();
			return over; } //returns true if not started!
		bool isOverTimeS(unsigned long limit = 0) { return isOverTime(limit * 1000);}
		bool isInTime(unsigned long limit = 0) { return _start && !isOverTime(limit); }
};
