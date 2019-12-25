#include <pybind11/embed.h>
#include <iostream>
#include "flexibity/log.h"

namespace py = pybind11;
using namespace py::literals;
using namespace Flexibity;

class PuzzleBase:
        public logInstanceName{
public:

    typedef std::shared_ptr<PuzzleBase> sPtr;

    PuzzleBase(){
        ILOG_INITSev(INFO);
        setInstanceName("defaultPuzzle");
    }

    bool isSolved(){
        return _solved;
    }

    bool test(){
        return _test;
    }

    Json::Value toJson(){
        auto v = Json::Value();

        v["name"] = instanceName();
        v["solved"] = isSolved();
        v["test"] = test();

        return v;
    }

    void solve(){
        _solved = true;
        onSolve();
    }

    virtual void onSolve() {}

    virtual bool onTest() { return false; }

    void reset(){
        _solved = false;
        onReset();
    }

    virtual void onReset() {}

    virtual int duration(){
        return 60;
    }

    void loop(bool runSolve = true){
        _test = onTest();
        if (runSolve && _test && !_solved)
            solve();
    }

protected:

    bool _solved = false;
    bool _test = false;

};

class PyLogInstanceName : public logInstanceName {
public:
    void setInstanceName(const std::string& n) override {
        PYBIND11_OVERLOAD(
            void,               /* Return type */
            logInstanceName,    /* Parent class */
            setInstanceName,    /* Name of function in C++ (must match Python name) */
            n                   /* Argument(s) */
        );
    }

    const std::string instanceName() override {
        PYBIND11_OVERLOAD(
            const std::string,  /* Return type */
            logInstanceName,    /* Parent class */
            instanceName        /* Name of function in C++ (must match Python name) */
        );
    }

//    const std::string name() override {

//    }
};

class PyPuzzleBase : public PuzzleBase {
public:
    using PuzzleBase::PuzzleBase;

    void onReset() override {
        PYBIND11_OVERLOAD(void, PuzzleBase, onReset,);
    }

    int duration() override {
        PYBIND11_OVERLOAD(int, PuzzleBase, duration,);
    }

    void onSolve() override {
        PYBIND11_OVERLOAD(void, PuzzleBase, onSolve,);
    }

    bool onTest() override {
        PYBIND11_OVERLOAD(bool, PuzzleBase, onTest,);
    }
};

PYBIND11_EMBEDDED_MODULE(cpp_module, m) {
    py::class_<logInstanceName>(m, "logInstanceName")
            .def("setInstanceName", &logInstanceName::setInstanceName)
            .def("instanceName", &logInstanceName::instanceName)
            .def("name", &logInstanceName::name);

    py::class_<PuzzleBase, PyPuzzleBase>(m, "PuzzleBase", py::multiple_inheritance())
            .def(py::init<>())
            .def("isSolved", &PuzzleBase::isSolved)
            .def("test", &PuzzleBase::test)
            .def("solve", &PuzzleBase::solve)
            .def("loop", &PuzzleBase::loop)
            .def("duration", &PuzzleBase::duration);
}

int main() {
    py::scoped_interpreter guard{};

    py::exec(R"(
        from cpp_module import *
        class Puzzle1(PuzzleBase):
             def __init__(self):
                PuzzleBase.__init__(self)
             def onSolve(self):
                print("onSolve")
             def onTest(self):
                true
             def duration(self):
                70
             def onReset(self):
                "reset"

        p = Puzzle1()
        p.solve()
        print("test")
    )", py::globals());

//    auto message = locals["message"].cast<std::string>();
//    std::cout << message;
}
