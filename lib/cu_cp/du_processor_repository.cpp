/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "du_processor_repository.h"
#include "srsran/cu_cp/cu_cp_configuration.h"
#include "srsran/cu_cp/du_processor_config.h"
#include "srsran/cu_cp/du_processor_factory.h"

using namespace srsran;
using namespace srs_cu_cp;

du_processor_repository::du_processor_repository(du_repository_config cfg_) :
  cfg(cfg_), logger(cfg.logger), du_task_sched(cfg.timers, *cfg.cu_cp.cu_cp_executor)
{
  f1ap_ev_notifier.connect_cu_cp(*this);
}

void du_processor_repository::handle_new_du_connection()
{
  du_index_t du_index = add_du();
  if (du_index == du_index_t::invalid) {
    logger.error("Rejecting new DU connection. Cause: Failed to create a new DU.");
    return;
  }

  logger.info("Added DU {}", du_index);
  if (cfg.amf_connected) {
    du_db.at(du_index).du_processor->get_rrc_amf_connection_handler().handle_amf_connection();
  }
}

void du_processor_repository::handle_du_remove_request(const du_index_t du_index)
{
  logger.info("removing DU {}", du_index);
  remove_du(du_index);
}

du_index_t du_processor_repository::add_du()
{
  du_index_t du_index = get_next_du_index();
  if (du_index == du_index_t::invalid) {
    logger.error("DU connection failed - maximum number of DUs connected ({})", MAX_NOF_DUS);
    return du_index_t::invalid;
  }

  // Create DU object
  auto it = du_db.insert(std::make_pair(du_index, du_context{}));
  srsran_assert(it.second, "Unable to insert DU in map");
  du_context& du_ctxt = it.first->second;
  du_ctxt.du_to_cu_cp_notifier.connect_cu_cp(cfg.cu_cp_du_handler, du_ctxt.ngap_du_processor_notifier);

  // TODO: use real config
  du_processor_config_t du_cfg = {};
  du_cfg.du_index              = du_index;
  du_cfg.rrc_cfg               = cfg.cu_cp.rrc_config;

  std::unique_ptr<du_processor_interface> du = create_du_processor(std::move(du_cfg),
                                                                   du_ctxt.du_to_cu_cp_notifier,
                                                                   f1ap_ev_notifier,
                                                                   *cfg.cu_cp.f1ap_notifier,
                                                                   cfg.e1ap_ctrl_notifier,
                                                                   cfg.ngap_ctrl_notifier,
                                                                   cfg.ue_nas_pdu_notifier,
                                                                   cfg.ue_ngap_ctrl_notifier,
                                                                   cfg.rrc_ue_cu_cp_notifier,
                                                                   cfg.ue_task_sched,
                                                                   cfg.ue_manager,
                                                                   cfg.cell_meas_mng,
                                                                   *cfg.cu_cp.cu_cp_executor);

  srsran_assert(du != nullptr, "Failed to create DU processor");
  du->get_context().du_index = du_index;
  du_ctxt.du_processor       = std::move(du);

  // Create connection DU processor to NGAP.
  du_ctxt.ngap_du_processor_notifier.connect_du_processor(du_ctxt.du_processor.get());

  return du_index;
}

du_index_t du_processor_repository::get_next_du_index()
{
  for (int du_idx_int = du_index_to_uint(du_index_t::min); du_idx_int < MAX_NOF_DUS; du_idx_int++) {
    du_index_t du_idx = uint_to_du_index(du_idx_int);
    if (du_db.find(du_idx) == du_db.end()) {
      return du_idx;
    }
  }
  return du_index_t::invalid;
}

void du_processor_repository::remove_du(du_index_t du_index)
{
  // Note: The caller of this function can be a DU procedure. Thus, we have to wait for the procedure to finish
  // before safely removing the DU. This is achieved via a scheduled async task

  srsran_assert(du_index != du_index_t::invalid, "Invalid du_index={}", du_index);
  logger.debug("Scheduling du_index={} deletion", du_index);

  // Schedule DU removal task
  du_task_sched.handle_du_async_task(
      du_index, launch_async([this, du_index](coro_context<async_task<void>>& ctx) {
        CORO_BEGIN(ctx);
        srsran_assert(du_db.find(du_index) != du_db.end(), "Remove DU called for inexistent du_index={}", du_index);

        // Remove DU
        du_db.erase(du_index);
        logger.info("Removed du_index={}", du_index);
        CORO_RETURN();
      }));
}

du_processor_interface& du_processor_repository::find_du(du_index_t du_index)
{
  srsran_assert(du_index != du_index_t::invalid, "Invalid du_index={}", du_index);
  srsran_assert(du_db.find(du_index) != du_db.end(), "DU not found du_index={}", du_index);
  return *du_db.at(du_index).du_processor;
}

f1ap_statistics_handler& du_processor_repository::get_f1ap_statistics_handler(du_index_t du_index)
{
  auto& du_it = find_du(du_index);
  return du_it.get_f1ap_statistics_handler();
}

size_t du_processor_repository::get_nof_dus() const
{
  return du_db.size();
}

size_t du_processor_repository::get_nof_ues() const
{
  size_t nof_ues = 0;
  for (auto& du : du_db) {
    nof_ues += du.second.du_processor->get_nof_ues();
  }
  return nof_ues;
}

f1ap_message_handler& du_processor_repository::get_f1ap_message_handler(du_index_t du_index)
{
  auto& du_it = find_du(du_index);
  return du_it.get_f1ap_message_handler();
}

void du_processor_repository::handle_amf_connection()
{
  // inform all connected DU objects about the new connection
  for (auto& du : du_db) {
    du.second.du_processor->get_rrc_amf_connection_handler().handle_amf_connection();
  }
}

void du_processor_repository::handle_amf_connection_drop()
{
  // inform all DU objects about the AMF connection drop
  for (auto& du : du_db) {
    du.second.du_processor->get_rrc_amf_connection_handler().handle_amf_connection_drop();
  }
}

void du_processor_repository::handle_paging_message(cu_cp_paging_message& msg)
{
  // Forward paging message to all DU processors
  for (auto& du : du_db) {
    du.second.du_processor->get_du_processor_paging_handler().handle_paging_message(msg);
  }
}

void du_processor_repository::request_ue_removal(du_index_t du_index, ue_index_t ue_index)
{
  du_db.at(du_index).du_processor->get_du_processor_ue_handler().remove_ue(ue_index);
}

void du_processor_repository::handle_inactivity_notification(du_index_t                           du_index,
                                                             const cu_cp_inactivity_notification& msg)
{
  // Forward message to DU processor
  du_db.at(du_index).du_processor->handle_inactivity_notification(msg);
}
