// Copyright (c) 2019 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "carla/trafficmanager/TrackTraffic.h"

namespace carla {
namespace traffic_manager {

namespace LocalizationConstants {
  const static uint64_t BUFFER_STEP_THROUGH = 10u;
} // namespace LocalizationConstants

  using namespace LocalizationConstants;

  TrackTraffic::TrackTraffic() {}

  void TrackTraffic::UpdateUnregisteredGridPosition(const ActorId actor_id, const SimpleWaypointPtr& waypoint) {

    std::lock_guard<std::mutex> lock(data_modification_mutex);

    // Add actor entry if not present.
    if (actor_to_grids.find(actor_id) == actor_to_grids.end()) {
      actor_to_grids.insert({actor_id, {}});
    }

    std::unordered_set<GeoGridId>& current_grids = actor_to_grids.at(actor_id);

    // Clear current actor from all grids containing current actor.
    for (auto& grid_id: current_grids) {
      if (grid_to_actors.find(grid_id) != grid_to_actors.end()) {
        ActorIdSet& actor_ids = grid_to_actors.at(grid_id);
        if (actor_ids.find(actor_id) != actor_ids.end()) {
          actor_ids.erase(actor_id);
        }
      }
    }

    // Clear all grids current actor is tracking.
    current_grids.clear();

    // Step through buffer and update grid list for actor and actor list for grids.
    if (waypoint != nullptr) {

      GeoGridId ggid = waypoint->GetGeodesicGridId();
      current_grids.insert(ggid);

      // Add grid entry if not present.
      if (grid_to_actors.find(ggid) == grid_to_actors.end()) {
        grid_to_actors.insert({ggid, {}});
      }

      ActorIdSet& actor_ids = grid_to_actors.at(ggid);
      if (actor_ids.find(actor_id) == actor_ids.end()) {
        actor_ids.insert(actor_id);
      }
    }
  }


  void TrackTraffic::UpdateGridPosition(const ActorId actor_id, const Buffer& buffer) {

    std::lock_guard<std::mutex> lock(data_modification_mutex);

    // Add actor entry if not present.
    if (actor_to_grids.find(actor_id) == actor_to_grids.end()) {
      actor_to_grids.insert({actor_id, {}});
    }

    std::unordered_set<GeoGridId>& current_grids = actor_to_grids.at(actor_id);

    // Clear current actor from all grids containing current actor.
    for (auto& grid_id: current_grids) {
      if (grid_to_actors.find(grid_id) != grid_to_actors.end()) {
        ActorIdSet& actor_ids = grid_to_actors.at(grid_id);
        if (actor_ids.find(actor_id) != actor_ids.end()) {
          actor_ids.erase(actor_id);
        }
      }
    }

    // Clear all grids current actor is tracking.
    current_grids.clear();

    // Step through buffer and update grid list for actor and actor list for grids.
    if (!buffer.empty()) {
      uint64_t buffer_size = buffer.size();
      uint64_t step_size = static_cast<uint64_t>(std::floor(buffer_size/BUFFER_STEP_THROUGH));
      for (uint64_t i = 0u; i <= BUFFER_STEP_THROUGH; ++i) {
        GeoGridId ggid = buffer.at(std::min(i* step_size, buffer_size-1u))->GetGeodesicGridId();
        current_grids.insert(ggid);

        // Add grid entry if not present.
        if (grid_to_actors.find(ggid) == grid_to_actors.end()) {
          grid_to_actors.insert({ggid, {}});
        }

        ActorIdSet& actor_ids = grid_to_actors.at(ggid);
        if (actor_ids.find(actor_id) == actor_ids.end()) {
          actor_ids.insert(actor_id);
        }
      }
    }
  }

  void TrackTraffic::UpdateGridPosition(const ActorId actor_id, const std::vector<SimpleWaypointPtr>& buffer) {

    std::lock_guard<std::mutex> lock(data_modification_mutex);

    // Add actor entry if not present.
    if (actor_to_grids.find(actor_id) == actor_to_grids.end()) {
      actor_to_grids.insert({actor_id, {}});
    }

    std::unordered_set<GeoGridId>& current_grids = actor_to_grids.at(actor_id);

    // Clear current actor from all grids containing current actor.
    for (auto& grid_id: current_grids) {
      if (grid_to_actors.find(grid_id) != grid_to_actors.end()) {
        ActorIdSet& actor_ids = grid_to_actors.at(grid_id);
        if (actor_ids.find(actor_id) != actor_ids.end()) {
          actor_ids.erase(actor_id);
        }
      }
    }

    // Clear all grids current actor is tracking.
    current_grids.clear();

    // Step through buffer and update grid list for actor and actor list for grids.
    if (!buffer.empty()) {
      for (const SimpleWaypointPtr& swp: buffer) {
        GeoGridId ggid = swp->GetGeodesicGridId();
        current_grids.insert(ggid);

        // Add grid entry if not present.
        if (grid_to_actors.find(ggid) == grid_to_actors.end()) {
          grid_to_actors.insert({ggid, {}});
        }

        ActorIdSet& actor_ids = grid_to_actors.at(ggid);
        if (actor_ids.find(actor_id) == actor_ids.end()) {
          actor_ids.insert(actor_id);
        }
      }
    }
  }

  ActorIdSet TrackTraffic::GetOverlappingVehicles(ActorId actor_id) {

    std::lock_guard<std::mutex> lock(data_modification_mutex);

    ActorIdSet actor_id_set;

    if (actor_to_grids.find(actor_id) != actor_to_grids.end()) {
      std::unordered_set<GeoGridId>& grid_ids = actor_to_grids.at(actor_id);
      for (auto& grid_id: grid_ids) {
        if (grid_to_actors.find(grid_id) != grid_to_actors.end()) {
          ActorIdSet& actor_ids = grid_to_actors.at(grid_id);
          actor_id_set.insert(actor_ids.begin(), actor_ids.end());
        }
      }
    }

    return actor_id_set;
  }

  void TrackTraffic::DeleteActor(ActorId actor_id) {

    std::lock_guard<std::mutex> lock(data_modification_mutex);

    if (actor_to_grids.find(actor_id) != actor_to_grids.end()) {
      std::unordered_set<GeoGridId>& grid_ids = actor_to_grids.at(actor_id);
      for (auto& grid_id: grid_ids) {
        if (grid_to_actors.find(grid_id) != grid_to_actors.end()) {
          ActorIdSet& actor_ids = grid_to_actors.at(grid_id);
          if (actor_ids.find(actor_id) != actor_ids.end()) {
            actor_ids.erase(actor_id);
          }
        }
      }
      actor_to_grids.erase(actor_id);
    }
  }

  std::unordered_set<GeoGridId> TrackTraffic::GetGridIds(ActorId actor_id) {

    std::unordered_set<GeoGridId> grid_ids;

    if (actor_to_grids.find(actor_id) != actor_to_grids.end()) {
      grid_ids = actor_to_grids.at(actor_id);
    }

    return grid_ids;
  }

  std::unordered_map<GeoGridId, ActorIdSet> TrackTraffic::GetGridActors() {
    return grid_to_actors;
  }

  void TrackTraffic::UpdatePassingVehicle(uint64_t waypoint_id, ActorId actor_id) {

    if (waypoint_overlap_tracker.find(waypoint_id) != waypoint_overlap_tracker.end()) {

      ActorIdSet& actor_id_set = waypoint_overlap_tracker.at(waypoint_id);
      if (actor_id_set.find(actor_id) == actor_id_set.end()) {
        actor_id_set.insert(actor_id);
      }
    } else {

      waypoint_overlap_tracker.insert({waypoint_id, {actor_id}});
    }
  }

  void TrackTraffic::RemovePassingVehicle(uint64_t waypoint_id, ActorId actor_id) {

    if (waypoint_overlap_tracker.find(waypoint_id) != waypoint_overlap_tracker.end()) {

      auto& actor_id_set = waypoint_overlap_tracker.at(waypoint_id);
      if (actor_id_set.find(actor_id) != actor_id_set.end()) {
        actor_id_set.erase(actor_id);
      }

      if (actor_id_set.size() == 0) {
        waypoint_overlap_tracker.erase(waypoint_id);
      }
    }
  }

  ActorIdSet TrackTraffic::GetPassingVehicles(uint64_t waypoint_id) {

    if (waypoint_overlap_tracker.find(waypoint_id) != waypoint_overlap_tracker.end()) {
      return waypoint_overlap_tracker.at(waypoint_id);
    } else {
      return ActorIdSet();
    }

  }

} // namespace traffic_manager
} // namespace carla
