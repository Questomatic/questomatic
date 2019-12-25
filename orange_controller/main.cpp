/*
 * Copyright (c) 2014, Peter Thorson. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the WebSocket++ Project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PETER THORSON BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** ====== WARNING ========
 * This example is presently used as a scratch space. It may or may not be broken
 * at any given time.
 */
#include <flexibity/jsonrpc/jsonRpcWebsocketServer.hpp>
#include <json/json.h>
#include <pybind11/embed.h>
#include <pybind11/functional.h>

#include <set>
#include "GPIO.hh"

#include "flexibity/programOptions.hpp"

#include <flexibity/jsonrpc/audioPlayMethod.hpp>
#include <flexibity/jsonrpc/audioStopMethod.hpp>
#include <flexibity/jsonrpc/genericMethod.hpp>

#include "modbus.hpp"
#include <chrono>

using namespace boost::asio;
using namespace Flexibity;
using namespace std::chrono;
namespace py = pybind11;
using namespace py::literals;

#ifdef __x86_64
#define _LOW GPIO::HIGH
#define _HIGH GPIO::LOW
#else
#define _LOW GPIO::LOW
#define _HIGH GPIO::HIGH
#endif

static Modbus::sPtr MODBUS;
static bool g_running = true;
static uint16_t port = 9012;
static std::string python_file = "/media/flash/scenario.py";
static std::string python_script; // save content here


class Mpg123Player:
	public logInstanceName{
	std::string _url;
	pid_t _pid;
public:

	Mpg123Player():
		_pid(0){
		ILOG_INITSev(INFO);
	}

	~Mpg123Player() {
		stop();
	}

	bool play(const std::string &url, uint16_t gain = 100){

		std::stringstream params;

		stop();

		auto gainStr = std::to_string(gain).c_str();

		const char *const argv[] = {"mpg123", "-a", "dmix", "--gain", gainStr,  url.c_str(), NULL};

		auto spawn_result = spawnUtil::run_cmd(argv, false);
		_pid = std::get<1>(spawn_result);

		IINFO("starting to play " << url << "; pid is: " << _pid);

		return false;
	}

	bool isPlaying() {
		std::stringstream process;
		process << "/proc/" << _pid << "/cmdline";
		std::ifstream lf(process.str());
		return lf.is_open();
	}

	void stop() {

		if(_pid == 0)
			return;

		IINFO("killing pid" << _pid);
		int k = kill(_pid, SIGKILL);
		if (k == 0 ) {
			IINFO("killing pid" << _pid << ": success");
			_pid = 0;
			return;
		}

		IERROR("killing pid" << _pid << ": failed");
	}
};

// Состояния: решена, не решена, взводится (тестируется)

class PuzzleBase:
		public logInstanceName{
public:
	using sPtr = PuzzleBase*;

	PuzzleBase(){
		ILOG_INITSev(INFO);
	}

	bool isSolved(){
		return _solved;
	}

	bool isRunning() {
		return _running;
	}

	bool test(){
		return _test;
	}

	int duration(){
		return _durationMs;
	}

	void setDuration(int d) { _durationMs = d; }

	Json::Value toJson(){
		auto v = Json::Value();

		v["name"] = instanceName();
		v["solved"] = isSolved();
		v["test"] = test();
		v["durationMs"] = duration();
		return v;
	}

	void solve(){
		if(_solved)
			return;

		_solved = true;
		_running = false;
		onSolve();
	}

	void stop(){
		_solved = true;
		_running = false;
		onStop();
	}

	void run() {
		_running = true;
		onRun();
	}

	virtual void onRun() {
		IINFO("");
	}

	virtual void onStop() {
		IINFO("");
	}

	virtual void onSolve() {
		IINFO("");
	}

	virtual bool onTest() {
		return false;
	}

	void arm(){
		_solved = false;
		_running = false;
		onArm();
	}

	virtual void onArm() {}

	void loop(bool runSolve = true){
		_test = onTest();
		if (runSolve && _test && !_solved)
			solve();
	}

protected:
	int  _durationMs = 1000 *30;
	bool _solved = false;
	bool _test = false;
	bool _running = false;
};

template <typename T>
string enumToString(T e, std::map<T, const char *> lookup){
	auto   it  = lookup.find(e);
	if (it == lookup.end())
		throw std::runtime_error("cannot convert " + std::to_string(e) + "of type " + typeid(T).name() + " to string");

	return it->second;
}

template <typename T>
T stringToEmum(string s, std::map<T, const char *> lookup){
	for (auto it = lookup.begin(); it != lookup.end(); ++it)
	{
		if (it->second == s)
		{
			return it->first;
		}
	}

	throw std::runtime_error("cannot convert " + s + " to enum of type" + typeid(T).name());
}


class Timeout {
public:
	Timeout(long duration){
		interval = milliseconds(duration);
		start = high_resolution_clock::now();
		end = start + interval;
	}

	long isOver() {
		auto elapsed = duration_cast<milliseconds>(high_resolution_clock::now() - end);
		long elapsedMs = elapsed.count();
		if (elapsedMs > 0) elapsedMs += (duration_cast<milliseconds>(interval)).count();
		return elapsedMs;
	}


	void reset() {
		start = high_resolution_clock::now();
		end = start + interval;
	}
private:
	milliseconds interval;
	time_point<high_resolution_clock> start;
	time_point<high_resolution_clock> end;
};

class QuestBase:
		public logInstanceName{
public:

	using sPtr = std::shared_ptr<QuestBase>;
	typedef enum {
		OFF,
		STOP,
		ARM,
		RUN
	}State;

	const std::map<State,const char*> StateStrings = {
		{State::OFF,  "OFF"},
		{State::STOP, "STOP"},
		{State::ARM,  "ARM"},
		{State::RUN,  "RUN"},
	};

	class setStateMethod: public genericMethod {
	public:
		static constexpr const char* command = "set_state";

		setStateMethod(QuestBase &qb):_qb(qb) {
			ILOG_INITSev(INFO);
		}

		virtual void invoke(
				  const Json::Value &params,
				  std::shared_ptr<Response> response) override {

			if (!response) {
				return; // if notification, do nothing
			}

			auto state = stringToEmum(params[0]["state"].asString(), _qb.StateStrings);

			switch (state){
			case State::OFF:
				_qb.off();
				break;
			case State::STOP:
				_qb.stop();
				break;
			case State::ARM:
				_qb.arm();
				break;
			case State::RUN:
				_qb.run();
				break;
			}

			response->sendResult(_qb.toJson());
		}

		QuestBase &_qb;
	};

	class nextMethod: public genericMethod {
	public:
		static constexpr const char* command = "next";

		nextMethod(QuestBase &qb):_qb(qb) {
			ILOG_INITSev(INFO);
		}

		virtual void invoke(
				  const Json::Value &params,
				  std::shared_ptr<Response> response) override {

			if (!response) {
				return; // if notification, do nothing
			}

			_qb.next();

			response->sendResult(_qb.toJson());
		}

		QuestBase &_qb;
	};

	class setScriptMethod: public genericMethod {
	public:
		static constexpr const char* command = "set_script";

		setScriptMethod(QuestBase &qb):_qb(qb){
			ILOG_INITSev(INFO);
			IINFO("setScriptMethod");
		}

		virtual void invoke(
				  const Json::Value &params,
				  std::shared_ptr<Response> response) override {
			IINFO(__PRETTY_FUNCTION__);
			if (!response) {
				return; // if notification, do nothing
			}

			std::string error;

			std::ofstream new_pyfile(python_file);
			IINFO("Saving quest contents to " << python_file);
			if(new_pyfile.is_open()) {
				auto script = params[0].asString();
				new_pyfile.write(script.c_str(), script.size());
				new_pyfile.close();
				sync();
				_qb.stopQuest();
			}

			if(!error.empty()) {
				GERROR(error);
				response->sendError(error);
				return;
			}

			response->sendResult(responseOK());
		}
		QuestBase &_qb;
	};

	QuestBase(jsonRpcWebsocketServer &ws): _ws(ws){
		ILOG_INITSev(INFO);
		_ws.addMethod(nextMethod::command, std::make_shared<nextMethod>(*this));
		_ws.addMethod(setStateMethod::command, std::make_shared<setStateMethod>(*this));
		_ws.addMethod(setScriptMethod::command, std::make_shared<setScriptMethod>(*this));
	}

	~QuestBase(){
		_ws.removeMethod(nextMethod::command);
		_ws.removeMethod(setStateMethod::command);
		_ws.removeMethod(setScriptMethod::command);
	}

	void loop(){

		static Timeout timeout = Timeout(_sendDelay);

		if(_log->sev() == log::severity::TRACE)
			usleep(1000000);

		ITRACE("enter");
		std::unique_lock<std::mutex> lck(_currentPuzzleLock);
		ITRACE("got lock, checking timeout");
		double a = timeout.isOver();
		if (a > 0){
			ITRACE("timeout is over");
			lck.unlock();
			tick(a);
			notifyStatus();
			timeout.reset();
			lck.lock();
		}


		ITRACE("checking next puzzle key");
		if(_nextPuzzle && _state != State::OFF){
			_nextPuzzle->loop();

			static auto nextPuzzleTestPrevState = false;

			auto nextPuzzleTest = _nextPuzzle->test();
			if(nextPuzzleTest && !nextPuzzleTestPrevState){
				IINFO("Triggering next puzzle by special key");
				lck.unlock();
				next();
				lck.lock();
			}

			nextPuzzleTestPrevState = nextPuzzleTest;
		}

		ITRACE("checking power puzzle key");
		if(_powerButtonPuzzle){
			_powerButtonPuzzle->loop();

			static auto powerButtonPuzzleTestPrevState = false;

			auto powerButtonPuzzleTest = _powerButtonPuzzle->test();
			if(powerButtonPuzzleTest && !powerButtonPuzzleTestPrevState){
				IINFO("Triggering power by special key:" << powerButtonPuzzleTest);

				lck.unlock();

				if(_state == State::STOP)
					off();
				else
					stop();

				lck.lock();
			}

			powerButtonPuzzleTestPrevState = powerButtonPuzzleTest;
		}

		ITRACE("checking service light puzzle key");
		if(_serviceLighButtonPuzzle && _serviceLighPuzzle && _state == State::RUN){
			_serviceLighButtonPuzzle->loop();

			static auto serviceLightButtonTestPrevState = false;

			auto serviceLightButtonPuzzleTest = _serviceLighButtonPuzzle->test();
			if(serviceLightButtonPuzzleTest && !serviceLightButtonTestPrevState){
				IINFO("Triggering Service Light by special key: " << serviceLightButtonPuzzleTest);

				if(!_serviceLighPuzzle->isSolved()){
					IINFO("Turning OFF service light by SL button");
					_serviceLighPuzzle->solve();
				}else{
					IINFO("Turning ON service light by SL button");
					_serviceLighPuzzle->arm();
				}

			}

			serviceLightButtonTestPrevState = serviceLightButtonPuzzleTest;
		}

		ITRACE("looping all puzzles");
		if(_state != State::OFF){
			for (auto i: _puzzles){
				i->loop(false); //test only
			}
		}

		ITRACE("checking current puzzle");
		if(_currentPuzzle != _puzzles.end()){
			auto p = *_currentPuzzle;

			p->loop(true);

			if(p->isSolved()){
				ITRACE("current puzzle solved, going to next");
				lck.unlock();
				next();
				lck.lock();
			}
		}

		ITRACE("leave");
	}

	Json::Value toJson() {
		auto root = Json::Value();

		root["quest_name"] = instanceName();

		auto puzzles = Json::Value(Json::arrayValue);

		for (auto i: _puzzles){
			puzzles.append(i->toJson());
		}

		root["puzzles"] = puzzles;
		root["state"] = enumToString(_state, StateStrings);
		root["elapsedQuestTime"] = _elapsedQuestTimeMs;
		root["elapsedPuzzleTime"] = _elapsedPuzzleTimeMs;
		root["totalQuestTime"] = _totalQuestTimeMs;
		//IINFO(root.toStyledString());
		return root;
	}

	void notifyStatus(){

		auto a = Json::Value(Json::arrayValue);
		a.append(toJson());

		_ws.notifyAll("status", a);
	}

	void addPuzzle(PuzzleBase::sPtr puzzle){
		std::unique_lock<std::mutex> lck(_currentPuzzleLock);
		_puzzles.push_back(puzzle);
		puzzle->stop();

		_totalQuestTimeMs += puzzle->duration();
	}

	void setSpecialPuzzles(
			PuzzleBase::sPtr offPuzzle,
			PuzzleBase::sPtr nextPuzzle,
			PuzzleBase::sPtr powerButtonPuzzle,
			PuzzleBase::sPtr serviceLightPuzzle,
			PuzzleBase::sPtr serviceLightButtonPuzzle){
		std::unique_lock<std::mutex> lck(_currentPuzzleLock);

		_offPuzzle = offPuzzle;
		_nextPuzzle = nextPuzzle;
		_powerButtonPuzzle = powerButtonPuzzle;
		_serviceLighPuzzle = serviceLightPuzzle;
		_serviceLighButtonPuzzle = serviceLightButtonPuzzle;

		if(_offPuzzle)
			_offPuzzle->solve();

		if(_nextPuzzle)
			_nextPuzzle->arm();

		if(_powerButtonPuzzle)
			_powerButtonPuzzle->arm();

		if(_serviceLighButtonPuzzle)
			_serviceLighButtonPuzzle->arm();

		if(_serviceLighPuzzle)
			_serviceLighPuzzle->arm();
	}

	void tick(double interval){

		if(_currentPuzzle == _puzzles.end())
			return;

		switch (_state){
		case State::RUN:
			_elapsedQuestTimeMs += interval;
			_elapsedPuzzleTimeMs += interval;
			if (_elapsedPuzzleTimeMs >= (*_currentPuzzle)->duration())
				next();
			if (_elapsedQuestTimeMs >= _totalQuestTimeMs) timeout();
			break;
		default:
			_elapsedQuestTimeMs = 0;
			_elapsedPuzzleTimeMs = 0;
		}

	}

	void next(){
		IINFO("");

		switch(_state){
		case State::STOP:
			arm();
			return;
		case State::ARM:
			run();
			return;
		}

		std::unique_lock<std::mutex> lck(_currentPuzzleLock);
		IINFO("got lock");

		if(_currentPuzzle == _puzzles.end()){
			return;
		}

		_elapsedPuzzleTimeMs = 0;
		auto nextPuzzle = _currentPuzzle + 1;

		(*_currentPuzzle)->solve();
		_currentPuzzle = nextPuzzle;

		if(nextPuzzle == _puzzles.end()){
			lck.unlock();
			stop();
			lck.lock();
		}else
			(*_currentPuzzle)->run();

	}

	void off(){
		IINFO("");
		std::unique_lock<std::mutex> lck(_currentPuzzleLock);
		IINFO("got lock");

		_currentPuzzle = _puzzles.end();

		_state = State::OFF;

		for (auto i: _puzzles)
			i->stop();

		if(_offPuzzle && !_offPuzzle->isSolved()){
			IINFO("Turning OFF quest power by special puzzle");
			_offPuzzle->solve();
		}
	}

	void arm(){
		IINFO("");
		std::unique_lock<std::mutex> lck(_currentPuzzleLock);
		IINFO("got lock");
		_state = State::ARM;

		for (auto i: _puzzles)
			i->arm();
	}

	void run(){
		IINFO("");
		std::unique_lock<std::mutex> lck(_currentPuzzleLock);
		IINFO("got lock");
		_state = State::RUN;
		_currentPuzzle = _puzzles.begin();

		IINFO("triggering run on current puzzle");
		if(_currentPuzzle != _puzzles.end())
			(*_currentPuzzle)->run();

		IINFO("checking service light");
		if(_serviceLighPuzzle && !_serviceLighPuzzle->isSolved()){
			IINFO("Turning OFF service light by special puzzle");
			_serviceLighPuzzle->solve();
		}

		IINFO("leave");
	}

	void stop(){
		IINFO("");
		std::unique_lock<std::mutex> lck(_currentPuzzleLock);
		IINFO("got lock");
		_state = State::STOP;

		_elapsedPuzzleTimeMs = 0;

		IINFO("Testing power puzzle");

		if(_offPuzzle && _offPuzzle->isSolved()){
			IINFO("Turning ON quest power by special puzzle");
			_offPuzzle->arm();
		}

		IINFO("Testing light puzzle");

		if(_serviceLighPuzzle && _serviceLighPuzzle->isSolved()){
			IINFO("Turning ON service light by special puzzle");
			_serviceLighPuzzle->arm();
		}

		IINFO("stopping all puzzles");

		for (auto i: _puzzles)
			i->stop();

		_currentPuzzle = _puzzles.end();

		IINFO("leave");
	}

	void timeout(){
		stop();
	}

	bool loadQuest(const std::string &q, std::string &error) {
		IINFO("");

		stopQuest();

		py::initialize_interpreter(false);

		try {
			IINFO("loaded python script");
			py::exec(q);

			isRunning = true;

			off();

			py::gil_scoped_release release;
			while(isRunning) {
				loop();
			}

			stopQuest();

			py::finalize_interpreter();
			IINFO("Leaving...");
			return true;
		}
		catch(std::exception& e) {
			error = e.what();
			IERROR(error);
			stopQuest();
			py::finalize_interpreter();
			IERROR("Leaving...");
			return false;
		}
	}

	void sigStop(){
		isRunning = false;
	}


	void stopQuest() {
		IINFO("");

		off();
		std::unique_lock<std::mutex> lck(_currentPuzzleLock);
		_totalQuestTimeMs = 0;
		_elapsedQuestTimeMs = 0;
		_elapsedPuzzleTimeMs = 0;

		isRunning = false;
		_puzzles.clear();
		_currentPuzzle = _puzzles.end();

		_offPuzzle = nullptr;
		_nextPuzzle = nullptr;
		_powerButtonPuzzle =  nullptr;
		_serviceLighPuzzle = nullptr;
		_serviceLighButtonPuzzle = nullptr;
	}

protected:
	bool isRunning = false;
	typedef vector<PuzzleBase::sPtr> Puzzles;
	Puzzles _puzzles;
	Puzzles::iterator _currentPuzzle = _puzzles.end();

	PuzzleBase::sPtr _offPuzzle = nullptr;
	PuzzleBase::sPtr _nextPuzzle = nullptr;
	PuzzleBase::sPtr _powerButtonPuzzle =  nullptr;
	PuzzleBase::sPtr _serviceLighPuzzle = nullptr;
	PuzzleBase::sPtr _serviceLighButtonPuzzle = nullptr;

	State _state = State::OFF;
	std::mutex _currentPuzzleLock;

	jsonRpcWebsocketServer & _ws;
	int const _sendDelay = 1000;

	double _elapsedQuestTimeMs = 0;
	double _elapsedPuzzleTimeMs = 0;
	double _totalQuestTimeMs = 0;
};

static QuestBase::sPtr qb;

void add_puzzle(PuzzleBase *pzl) {
	if(!pzl)
		return;

	GINFO("Adding puzzle " << pzl->instanceName() << " from Python");
	qb->addPuzzle(pzl);
}

void set_quest_name(const std::string& name) {
	qb->setInstanceName(name);
}

void set_special_puzzles(PuzzleBase::sPtr offPuzzle,
						 PuzzleBase::sPtr nextPuzzle,
						 PuzzleBase::sPtr powerButtonPuzzle,
						 PuzzleBase::sPtr serviceLightPuzzle,
						 PuzzleBase::sPtr serviceLightButtonPuzzle) {
	GINFO("Setting Special Puzzles from python");
	qb->setSpecialPuzzles(offPuzzle, nextPuzzle, powerButtonPuzzle, serviceLightPuzzle, serviceLightButtonPuzzle);
}

class PyPuzzleBase : public PuzzleBase {
public:
	using PuzzleBase::PuzzleBase;

	void onRun() override {
		PYBIND11_OVERLOAD(void, PuzzleBase, onRun,);
	}

	void onStop() override {
		PYBIND11_OVERLOAD(void, PuzzleBase, onStop,);
	}

	void onSolve() override {
		PYBIND11_OVERLOAD(void, PuzzleBase, onSolve,);
	}

	bool onTest() override {
		PYBIND11_OVERLOAD(bool, PuzzleBase, onTest,);
	}

	void onArm() override {
		PYBIND11_OVERLOAD(void, PuzzleBase, onArm,);
	}
};

PYBIND11_EMBEDDED_MODULE(quest_module, m) {
	py::class_<GPIO> GPIOp(m, "GPIO");

	GPIOp.def(py::init<unsigned short>())
			.def(py::init<unsigned short, GPIO::Type, GPIO::isr_callback>())
			.def("setValue", [](GPIO& self, const GPIO::Value value){
				GDEBUG("Setting GPIO " << self.id() << " with value " << value);
				self.setValue(value);
			}, py::call_guard<py::gil_scoped_release>())
			.def("getValue", &GPIO::getValue, py::call_guard<py::gil_scoped_release>())
			.def("setHigh", [](GPIO &self) {
				self.setValue(GPIO::Value::HIGH);
			})
			.def("setLow", [](GPIO &self) {
				self.setValue(GPIO::Value::HIGH);
			})
			.def_property_readonly("isHigh", [](GPIO &self){
				return self.getValue() == GPIO::Value::HIGH;
			})
			.def_property_readonly("isLow", [](GPIO &self){
				return self.getValue() == GPIO::Value::LOW;
			})
			.def("id", &GPIO::id, py::call_guard<py::gil_scoped_release>());

	py::enum_<GPIO::Type>(GPIOp, "Type")
			.value("OUT",       GPIO::Type::OUT)
			.value("IN",        GPIO::Type::IN)
			.value("RISING",    GPIO::Type::RISING)
			.value("FALLING",   GPIO::Type::FALLING)
			.value("BOTH",      GPIO::Type::BOTH)
			.export_values();

	py::enum_<GPIO::Value>(GPIOp, "Value")
			.value("LOW", GPIO::Value::LOW)
			.value("HIGH", GPIO::Value::HIGH)
			.export_values();

	py::class_<logInstanceName>(m, "logInstanceName")
			.def("setInstanceName", &logInstanceName::setInstanceName)
			.def("instanceName", &logInstanceName::instanceName)
			.def("name", &logInstanceName::name);

	py::class_<PuzzleBase, PyPuzzleBase, logInstanceName>(m, "PuzzleBase", py::multiple_inheritance())
			.def(py::init<>())
			.def("isSolved", &PuzzleBase::isSolved, py::call_guard<py::gil_scoped_release>())
			.def("isRunning", &PuzzleBase::isRunning, py::call_guard<py::gil_scoped_release>())
			.def("test", &PuzzleBase::test, py::call_guard<py::gil_scoped_release>())
			.def("solve", &PuzzleBase::solve, py::call_guard<py::gil_scoped_release>())
			.def("loop", &PuzzleBase::loop, py::call_guard<py::gil_scoped_release>())
			.def("duration", &PuzzleBase::duration, py::call_guard<py::gil_scoped_release>())
			.def("setDuration", &PuzzleBase::setDuration, py::call_guard<py::gil_scoped_release>());

	py::class_<Mpg123Player, logInstanceName>(m, "Mpg123Player", py::multiple_inheritance())
			.def(py::init<>())
			.def("play", &Mpg123Player::play, py::arg("url"), py::arg("gain") = 100, py::call_guard<py::gil_scoped_release>())
			.def("stop", &Mpg123Player::stop, py::call_guard<py::gil_scoped_release>())
			.def("isPlaying", &Mpg123Player::isPlaying, py::call_guard<py::gil_scoped_release>());

	py::class_<Modbus>(m, "Modbus")
			.def(py::init<const std::string&>())
			.def("WriteRegister", &Modbus::WriteRegister, py::call_guard<py::gil_scoped_release>())
			.def("ReadRegisters", [](Modbus &self, int slaveAddr, int addr) {
				uint16_t data;
				auto mb = self.ReadRegisters(slaveAddr, addr, 1, &data);
				return py::make_tuple(mb, data);
			}, py::call_guard<py::gil_scoped_release>())
			.def("WriteRegisters", [](Modbus &self, int slaveAddr, int addr, py::object o){
				std::vector<uint16_t> data;
				std::string input_string = py::str(o);
				for(size_t i = 0; i < input_string.length(); i++) {
					data.push_back(static_cast<uint16_t>(static_cast<uint8_t>(input_string[i])));
				}

				return self.WriteRegisters(slaveAddr, addr, data.size(), &data[0]);
			}, py::call_guard<py::gil_scoped_release>());

	py::class_<Timeout>(m, "Timeout")
			.def(py::init<long>())
			.def("isOver", &Timeout::isOver)
			.def("reset", &Timeout::reset);

	m.def("add_puzzle", &add_puzzle, py::call_guard<py::gil_scoped_release>());
	m.def("set_quest_name", &set_quest_name, py::call_guard<py::gil_scoped_release>());
	m.def("set_special_puzzles", &set_special_puzzles, py::call_guard<py::gil_scoped_release>());
	m.def("usleep", &usleep);
}

void sig_interceptor(int sig){ // can be called asynchronously
	GWARN("signal " << sig << " caught");
	qb->sigStop();
	g_running = false;
}

int main(int argc, char **argv ) {

	Flexibity::programOptions options;

	signal(SIGINT, sig_interceptor);
	signal(SIGTSTP, sig_interceptor);
	signal(SIGQUIT, sig_interceptor);

	// prevent zombies from mpg123player
	struct sigaction arg;
	arg.sa_handler = SIG_IGN;
	arg.sa_flags = SA_NOCLDWAIT; // Never wait for termination of a child process.

	sigaction(SIGCHLD, &arg, NULL);

	options.desc.add_options()
			("port,p",  Flexibity::po::value<uint16_t>(&port), "Define the WebSocket port to bind to.")
			("script,s", Flexibity::po::value<std::string>(&python_file));

	options.parse(argc, argv);

	Json::Value cfg;
	cfg[jsonRpcWebsocketServer::portOption] = port;

	GINFO("Starting websocket server on " << port << " port");
	jsonRpcWebsocketServer server(cfg);

	//TODO: implement file server
	server.onHttp.connect([](jsonRpcWebsocketServer::server::connection_ptr con)-> websocketpp::http::status_code::value {
		GINFO(con->get_resource());
		con->set_body("hello!");
		return websocketpp::http::status_code::ok;
	});

	qb = make_shared<QuestBase>(server);


	while(g_running){
		std::string error;
		std::ifstream script_stream(python_file);
		GINFO("trying to load quest from " << python_file);
		if (script_stream.is_open()) {
			python_script.assign((std::istreambuf_iterator<char>(script_stream)),
							 std::istreambuf_iterator<char>());
			script_stream.close();

			qb->loadQuest(python_script, error);
			GERROR(error);
		}else
			GINFO("Unable to load quest from " << python_file);

		sleep(5);
	}

	server.stop();
	server.join();

	GINFO("terminating");
}
