// P27 - State (about 30 min).
//
// A diagnostic session, done with the State pattern: SessionMachine delegates
// every request and every event to the current state object. Transitions swap
// that object. Do not put a switch on the session inside SessionMachine.
//
// No heap. The two state objects live for the whole program.
//
// Starts in Default.
//   request(Read)         true in both sessions
//   request(Clear)        true only in Extended; does not change the session
//   enter_extended()      Default -> Extended; already Extended stays there
//   tick_timeout()        Extended -> Default; already Default stays there
//
// The machine exposes change_state() so a state can name the state it moves to.
#include "check.hpp"

#include <type_traits>

// ===== YOUR CODE =====

enum class Session { Default, Extended };
enum class Service { Read, Clear };

class SessionMachine;

class ISessionState {
public:
    virtual ~ISessionState() = default;
    ISessionState(const ISessionState&) = delete;
    ISessionState& operator=(const ISessionState&) = delete;

    virtual Session id() const = 0;
    virtual bool request(Service service) = 0;
    virtual void enter_extended(SessionMachine& machine) = 0;
    virtual void tick_timeout(SessionMachine& machine) = 0;

protected:
    ISessionState() = default;
};

class SessionMachine {
public:
    SessionMachine();

    Session session() const { return state_->id(); }
    void enter_extended() { state_->enter_extended(*this); }
    void tick_timeout() { state_->tick_timeout(*this); }
    bool request(Service service) { return state_->request(service); }

    void change_state(ISessionState& next) { state_ = &next; }

private:
    ISessionState* state_;
};

class DefaultState : public ISessionState {
public:
    Session id() const override { return Session::Default; }
    bool request(Service service) override 
    {
        if(service == Service::Read)
        {
            return true;
        }
        return false; 
    }
    void enter_extended(SessionMachine& machine) override;
    void tick_timeout(SessionMachine&) override {}
};

class ExtendedState : public ISessionState {
public:
    Session id() const override { return Session::Extended; }
    bool request(Service) override { return true; }
    void enter_extended(SessionMachine&) override {}
    void tick_timeout(SessionMachine&) override;
};

DefaultState& default_state1()
{
    static DefaultState state;
    return state;
}
ExtendedState& extended_state()
{
    static ExtendedState state;
    return state;
}

void DefaultState::enter_extended(SessionMachine& machine)
{
    machine.change_state(extended_state());
}

void ExtendedState::tick_timeout(SessionMachine& machine)
{
    machine.change_state(default_state1());
}


namespace {

DefaultState& default_state() {
    static DefaultState state;
    return state;
}

}  // namespace

SessionMachine::SessionMachine() : state_(&default_state()) {}

// ===== TESTS (do not edit) =====

int main() {
    static_assert(std::has_virtual_destructor_v<ISessionState>);
    static_assert(std::is_abstract_v<ISessionState>);

    SessionMachine machine;
    CHECK(machine.session() == Session::Default);
    CHECK(machine.request(Service::Read));
    CHECK(!machine.request(Service::Clear));
    CHECK(machine.session() == Session::Default);
    machine.tick_timeout();
    CHECK(machine.session() == Session::Default);

    machine.enter_extended();
    CHECK(machine.session() == Session::Extended);
    CHECK(machine.request(Service::Read));
    CHECK(machine.request(Service::Clear));
    CHECK(machine.session() == Session::Extended);
    machine.enter_extended();
    CHECK(machine.session() == Session::Extended);

    machine.tick_timeout();
    CHECK(machine.session() == Session::Default);
    CHECK(!machine.request(Service::Clear));
    CHECK(machine.request(Service::Read));

    machine.enter_extended();
    machine.tick_timeout();
    machine.tick_timeout();
    CHECK(machine.session() == Session::Default);
    CHECK(!machine.request(Service::Clear));
    return report();
}
