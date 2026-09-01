/**
 * @file action.h
 * @brief An action = 3 services + 2 topics + a bounded goal table + a
 *        pure state-transition function. No new primitive.
 *
 * Mirrors rcl_action's own structure: `rcl_action_client_impl_t` is
 * literally { goal_client, cancel_client, result_client : rcl_client_t;
 * feedback_subscription, status_subscription : rcl_subscription_t }. The
 * goal lifecycle is `rcl_action_transition_goal_state(state, event)` — a
 * pure function that constant-folds to a branch cascade here.
 */
#pragma once
#include "rosCompose/service.h"

namespace rosCompose {

  enum class GoalState { Unknown = 0, Accepted, Executing, Canceling,
                         Succeeded, Canceled, Aborted };
  enum class GoalEvent { Execute = 0, CancelGoal, Succeed, Abort, Canceled };

  constexpr GoalState transition(GoalState s, GoalEvent e) {
    switch (s) {
      case GoalState::Accepted:
        if (e == GoalEvent::Execute)    return GoalState::Executing;
        if (e == GoalEvent::CancelGoal) return GoalState::Canceling;
        if (e == GoalEvent::Succeed)    return GoalState::Succeeded;
        if (e == GoalEvent::Abort)      return GoalState::Aborted;
        break;
      case GoalState::Executing:
        if (e == GoalEvent::CancelGoal) return GoalState::Canceling;
        if (e == GoalEvent::Succeed)    return GoalState::Succeeded;
        if (e == GoalEvent::Abort)      return GoalState::Aborted;
        break;
      case GoalState::Canceling:
        if (e == GoalEvent::Canceled)   return GoalState::Canceled;
        if (e == GoalEvent::Succeed)    return GoalState::Succeeded;
        if (e == GoalEvent::Abort)      return GoalState::Aborted;
        break;
      default: break;
    }
    return GoalState::Unknown;
  }

  /// Assembled from Service + PubLink + a fixed goal array + transition().
  template<typename Goal, typename Result, typename Feedback, int MaxGoals>
  struct ActionServer {
    Service<Goal, int>    goalSrv;     ///< -> accepted goal id (0 = reject)
    Service<int,  Result> resultSrv;   ///< goal id -> result
    Service<int,  bool>   cancelSrv;   ///< goal id -> accepted
    PubLink<Feedback>     feedbackPub;
    PubLink<int>          statusPub;   ///< goal state as int

    struct GoalSlot { int id; GoalState state; bool used; };
    GoalSlot goals[MaxGoals] = {};
    int nextId = 1;

    int accept(const Goal&) {
      for (auto& g : goals) if (!g.used) { g = {nextId++, GoalState::Accepted, true}; return g.id; }
      return 0;
    }
    void advance(int id, GoalEvent e) {
      for (auto& g : goals) if (g.used && g.id == id) {
        g.state = transition(g.state, e);
        if (statusPub.link) statusPub.link->deliver((int)g.state);
        if (g.state == GoalState::Succeeded || g.state == GoalState::Canceled ||
            g.state == GoalState::Aborted) g.used = false;
        return;
      }
    }
    void feedback(const Feedback& f) { if (feedbackPub.link) feedbackPub.link->deliver(f); }
    GoalState stateOf(int id) const {
      for (auto& g : goals) if (g.used && g.id == id) return g.state;
      return GoalState::Unknown;
    }
  };

}
