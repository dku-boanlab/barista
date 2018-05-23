/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

/** \brief The macro for the main function of a component */
#define DECLARE_MAIN_FUNC(name) \
    extern int name(int *activated, int argc, char **argv)
/** \brief The macro for the handler function of a component */
#define DECLARE_HANDLER_FUNC(name) \
    extern int name(const event_t *ev, event_out_t *ev_out)
/** \brief The macro for the cleanup function of a component */
#define DECLARE_CLEANUP_FUNC(name) \
    extern int name(int *activated)
/** \brief The macro for the CLI function of a component */
#define DECLARE_CLI_FUNC(name) \
    extern int name(cli_t *cli, char **args)

DECLARE_MAIN_FUNC(log_main);
DECLARE_HANDLER_FUNC(log_handler);
DECLARE_CLEANUP_FUNC(log_cleanup);
DECLARE_CLI_FUNC(log_cli);

DECLARE_MAIN_FUNC(conn_main);
DECLARE_HANDLER_FUNC(conn_handler);
DECLARE_CLEANUP_FUNC(conn_cleanup);
DECLARE_CLI_FUNC(conn_cli);

DECLARE_MAIN_FUNC(conn_sb_main);
DECLARE_HANDLER_FUNC(conn_sb_handler);
DECLARE_CLEANUP_FUNC(conn_sb_cleanup);
DECLARE_CLI_FUNC(conn_sb_cli);

DECLARE_MAIN_FUNC(ofp10_main);
DECLARE_HANDLER_FUNC(ofp10_handler);
DECLARE_CLEANUP_FUNC(ofp10_cleanup);
DECLARE_CLI_FUNC(ofp10_cli);

DECLARE_MAIN_FUNC(switch_mgmt_main);
DECLARE_HANDLER_FUNC(switch_mgmt_handler);
DECLARE_CLEANUP_FUNC(switch_mgmt_cleanup);
DECLARE_CLI_FUNC(switch_mgmt_cli);

DECLARE_MAIN_FUNC(host_mgmt_main);
DECLARE_HANDLER_FUNC(host_mgmt_handler);
DECLARE_CLEANUP_FUNC(host_mgmt_cleanup);
DECLARE_CLI_FUNC(host_mgmt_cli);

DECLARE_MAIN_FUNC(topo_mgmt_main);
DECLARE_HANDLER_FUNC(topo_mgmt_handler);
DECLARE_CLEANUP_FUNC(topo_mgmt_cleanup);
DECLARE_CLI_FUNC(topo_mgmt_cli);

DECLARE_MAIN_FUNC(flow_mgmt_main);
DECLARE_HANDLER_FUNC(flow_mgmt_handler);
DECLARE_CLEANUP_FUNC(flow_mgmt_cleanup);
DECLARE_CLI_FUNC(flow_mgmt_cli);

DECLARE_MAIN_FUNC(stat_mgmt_main);
DECLARE_HANDLER_FUNC(stat_mgmt_handler);
DECLARE_CLEANUP_FUNC(stat_mgmt_cleanup);
DECLARE_CLI_FUNC(stat_mgmt_cli);

DECLARE_MAIN_FUNC(traffic_mgmt_main);
DECLARE_HANDLER_FUNC(traffic_mgmt_handler);
DECLARE_CLEANUP_FUNC(traffic_mgmt_cleanup);
DECLARE_CLI_FUNC(traffic_mgmt_cli);

DECLARE_MAIN_FUNC(resource_mgmt_main);
DECLARE_HANDLER_FUNC(resource_mgmt_handler);
DECLARE_CLEANUP_FUNC(resource_mgmt_cleanup);
DECLARE_CLI_FUNC(resource_mgmt_cli);

DECLARE_MAIN_FUNC(apphdlr_main);
DECLARE_HANDLER_FUNC(apphdlr_handler);
DECLARE_CLEANUP_FUNC(apphdlr_cleanup);
DECLARE_CLI_FUNC(apphdlr_cli);

DECLARE_MAIN_FUNC(cluster_main);
DECLARE_HANDLER_FUNC(cluster_handler);
DECLARE_CLEANUP_FUNC(cluster_cleanup);
DECLARE_CLI_FUNC(cluster_cli);

DECLARE_MAIN_FUNC(cac_main);
DECLARE_HANDLER_FUNC(cac_handler);
DECLARE_CLEANUP_FUNC(cac_cleanup);
DECLARE_CLI_FUNC(cac_cli);

DECLARE_MAIN_FUNC(dit_main);
DECLARE_HANDLER_FUNC(dit_handler);
DECLARE_CLEANUP_FUNC(dit_cleanup);
DECLARE_CLI_FUNC(dit_cli);

DECLARE_MAIN_FUNC(ofp10_veri_main);
DECLARE_HANDLER_FUNC(ofp10_veri_handler);
DECLARE_CLEANUP_FUNC(ofp10_veri_cleanup);
DECLARE_CLI_FUNC(ofp10_veri_cli);

DECLARE_MAIN_FUNC(conflict_main);
DECLARE_HANDLER_FUNC(conflict_handler);
DECLARE_CLEANUP_FUNC(conflict_cleanup);
DECLARE_CLI_FUNC(conflict_cli);

/** \brief The function pointer of a component */
struct _compnt_func_t {
    char *name; /**< Component name */
    compnt_main_f main; /**< The main function pointer */
    compnt_handler_f handler; /**< The handler function pointer */
    compnt_cleanup_f cleanup; /**< The cleanup function pointer */
    compnt_cli_f cli; /**< The CLI function pointer */
} g_components[] = {
    {"log", log_main, log_handler, log_cleanup, log_cli},
    {"conn", conn_main, conn_handler, conn_cleanup, conn_cli},
    {"conn_sb", conn_sb_main, conn_sb_handler, conn_sb_cleanup, conn_sb_cli},
    {"ofp10", ofp10_main, ofp10_handler, ofp10_cleanup, ofp10_cli},
    {"switch_mgmt", switch_mgmt_main, switch_mgmt_handler, switch_mgmt_cleanup, switch_mgmt_cli},
    {"host_mgmt", host_mgmt_main, host_mgmt_handler, host_mgmt_cleanup, host_mgmt_cli},
    {"topo_mgmt", topo_mgmt_main, topo_mgmt_handler, topo_mgmt_cleanup, topo_mgmt_cli},
    {"flow_mgmt", flow_mgmt_main, flow_mgmt_handler, flow_mgmt_cleanup, flow_mgmt_cli},
    {"stat_mgmt", stat_mgmt_main, stat_mgmt_handler, stat_mgmt_cleanup, stat_mgmt_cli},
    {"traffic_mgmt", traffic_mgmt_main, traffic_mgmt_handler, traffic_mgmt_cleanup, traffic_mgmt_cli},
    {"resource_mgmt", resource_mgmt_main, resource_mgmt_handler, resource_mgmt_cleanup, resource_mgmt_cli},
    {"apphdlr", apphdlr_main, apphdlr_handler, apphdlr_cleanup, apphdlr_cli},
    {"cluster", cluster_main, cluster_handler, cluster_cleanup, cluster_cli},
    {"cac", cac_main, cac_handler, cac_cleanup, cac_cli},
    {"dit", dit_main, dit_handler, dit_cleanup, dit_cli},
    {"ofp10_veri", ofp10_veri_main, ofp10_veri_handler, ofp10_veri_cleanup, ofp10_veri_cli},
    {"conflict", conflict_main, conflict_handler, conflict_cleanup, conflict_cli},
}; /**< The list of function pointers */
