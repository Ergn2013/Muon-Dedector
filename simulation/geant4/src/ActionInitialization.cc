#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"
#include "SteppingAction.hh"

namespace B1 {

void ActionInitialization::BuildForMaster() const {
  SetUserAction(new RunAction);
}

void ActionInitialization::Build() const {
  SetUserAction(new PrimaryGeneratorAction);
  SetUserAction(new RunAction);

  // Eşzamanlılık (Coincidence) Sayacımızı Bağlıyoruz!
  EventAction* eventAction = new EventAction();
  SetUserAction(eventAction);
  SetUserAction(new SteppingAction(eventAction));
}
} // namespace B1
