// ... keep all prior includes and code above unchanged ...

void ARGBStripComponent::set_arm_select_mode(ArmSelectMode m) {
  if (arm_select_mode_ == m) return;

  // Determine previous and new target groups
  std::string prev_group = get_arm_select_group_name_();
  std::string new_group;
  switch (m) {
    case ArmSelectMode::AWAY: new_group = "away"; break;
    case ArmSelectMode::HOME: new_group = "home"; break;
    case ArmSelectMode::DISARM: new_group = "disarm"; break;
    case ArmSelectMode::NIGHT:
    case ArmSelectMode::VACATION:
    case ArmSelectMode::BYPASS:
      new_group = "custom"; break;
    default: /* NONE */ break;
  }

  // Apply pending writes if:
  //  - We were in an arm select mode (prev_group not empty), AND
  //    (a) we are leaving arm select entirely (m == NONE), OR
  //    (b) the target group is changing (prev_group != new_group).
  if (arm_select_mode_ != ArmSelectMode::NONE && !prev_group.empty()) {
    if (m == ArmSelectMode::NONE || prev_group != new_group) {
      apply_pending_for_group_(prev_group);
    }
  }

  // Update mode
  arm_select_mode_ = m;

  // If entering (or switching to) an arm select mode with a different group, (re)initialize pending capture.
  if (arm_select_mode_ != ArmSelectMode::NONE && !new_group.empty()) {
    // If group changed we start a fresh pending buffer; if same group we keep existing deferred writes.
    if (prev_group != new_group) {
      auto &pend = pending_writes_[new_group];
      pend.used = true;
      pend.channel_set = {false,false,false};
    }
  }

  // If RFID visuals are active, we suppress overlay but still updated internal mode state.
  if (!rfid_visual_active_()) {
    mark_dirty_();
  }
}

// ... keep the rest of file unchanged ...
