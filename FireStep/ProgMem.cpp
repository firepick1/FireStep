#include "Arduino.h"
#include "ProgMem.h"
#include "Machine.h"

using namespace firestep;

const char firestep::OP_1[] PROGMEM = { "1" };
const char firestep::OP_2[] PROGMEM = { "2" };
const char firestep::OP_3[] PROGMEM = { "3" };
const char firestep::OP_4[] PROGMEM = { "4" };
const char firestep::OP_a1[] PROGMEM = { "a1" };
const char firestep::OP_a2[] PROGMEM = { "a2" };
const char firestep::OP_a3[] PROGMEM = { "a3" };
const char firestep::OP_a[] PROGMEM = { "a" };
const char firestep::OP_ah[] PROGMEM = { "ah" };
const char firestep::OP_angle[] PROGMEM = { "angle" };
const char firestep::OP_as[] PROGMEM = { "as" };
const char firestep::OP_ax[] PROGMEM = { "ax" };
const char firestep::OP_ay[] PROGMEM = { "ay" };
const char firestep::OP_az[] PROGMEM = { "az" };
const char firestep::OP_b[] PROGMEM = { "b" };
const char firestep::OP_bx[] PROGMEM = { "bx" };
const char firestep::OP_by[] PROGMEM = { "by" };
const char firestep::OP_bz[] PROGMEM = { "bz" };
const char firestep::OP_c[] PROGMEM = { "c" };
const char firestep::OP_cal[] PROGMEM = { "cal" };
const char firestep::OP_cal_coarse[] PROGMEM = { "cal-coarse" };
const char firestep::OP_cal_fine[] PROGMEM = { "cal-fine" };
const char firestep::OP_cal_fpd_bed_coarse[] PROGMEM = { "cal-fpd-bed-coarse" };
const char firestep::OP_cal_fpd_bed_fine[] PROGMEM = { "cal-fpd-bed-fine" };
const char firestep::OP_cal_fpd_bed_medium[] PROGMEM = { "cal-fpd-bed-medium" };
const char firestep::OP_cal_fpd_home_coarse[] PROGMEM = { "cal-fpd-home-coarse" };
const char firestep::OP_cal_fpd_home_fine[] PROGMEM = { "cal-fpd-home-fine" };
const char firestep::OP_cal_fpd_home_medium[] PROGMEM = { "cal-fpd-home-medium" };
const char firestep::OP_cal_gear[] PROGMEM = { "cal-gear" };
const char firestep::OP_cal_gear_coarse[] PROGMEM = { "cal-gear-coarse" };
const char firestep::OP_cal_gear_fine[] PROGMEM = { "cal-gear-fine" };
const char firestep::OP_calbx[] PROGMEM = { "calbx" };
const char firestep::OP_calby[] PROGMEM = { "calby" };
const char firestep::OP_calbz[] PROGMEM = { "calbz" };
const char firestep::OP_calge[] PROGMEM = { "calge" };
const char firestep::OP_calgr1[] PROGMEM = { "calgr1" };
const char firestep::OP_calgr2[] PROGMEM = { "calgr2" };
const char firestep::OP_calgr3[] PROGMEM = { "calgr3" };
const char firestep::OP_calgr[] PROGMEM = { "calgr" };
const char firestep::OP_calha[] PROGMEM = { "calha" };
const char firestep::OP_calhe[] PROGMEM = { "calhe" };
const char firestep::OP_calsv[] PROGMEM = { "calsv" };
const char firestep::OP_calzc[] PROGMEM = { "calzc" };
const char firestep::OP_calzr[] PROGMEM = { "calzr" };
const char firestep::OP_cb[] PROGMEM = { "cb" };
const char firestep::OP_cg[] PROGMEM = { "cg" };
const char firestep::OP_ch[] PROGMEM = { "ch" };
const char firestep::OP_cmt[] PROGMEM = { "cmt" };
const char firestep::OP_cr[] PROGMEM = { "cr" };
const char firestep::OP_d[] PROGMEM = { "d" };
const char firestep::OP_db[] PROGMEM = { "db" };
const char firestep::OP_dh[] PROGMEM = { "dh" };
const char firestep::OP_dim[] PROGMEM = { "dim" };
const char firestep::OP_dim_fpd[] PROGMEM = { "dim-fpd" };
const char firestep::OP_dimbx[] PROGMEM = { "dimbx" };
const char firestep::OP_dimby[] PROGMEM = { "dimby" };
const char firestep::OP_dimbz[] PROGMEM = { "dimbz" };
const char firestep::OP_dime[] PROGMEM = { "dime" };
const char firestep::OP_dimf[] PROGMEM = { "dimf" };
const char firestep::OP_dimgr1[] PROGMEM = { "dimgr1" };
const char firestep::OP_dimgr2[] PROGMEM = { "dimgr2" };
const char firestep::OP_dimgr3[] PROGMEM = { "dimgr3" };
const char firestep::OP_dimgr[] PROGMEM = { "dimgr" };
const char firestep::OP_dimha1[] PROGMEM = { "dimha1" };
const char firestep::OP_dimha2[] PROGMEM = { "dimha2" };
const char firestep::OP_dimha3[] PROGMEM = { "dimha3" };
const char firestep::OP_dimha[] PROGMEM = { "dimha" };
const char firestep::OP_dimhp[] PROGMEM = { "dimhp" };
const char firestep::OP_dimmi[] PROGMEM = { "dimmi" };
const char firestep::OP_dimpd[] PROGMEM = { "dimpd" };
const char firestep::OP_dimre[] PROGMEM = { "dimre" };
const char firestep::OP_dimrf[] PROGMEM = { "dimrf" };
const char firestep::OP_dimspa[] PROGMEM = { "dimspa" };
const char firestep::OP_dimspr[] PROGMEM = { "dimspr" };
const char firestep::OP_dimst[] PROGMEM = { "dimst" };
const char firestep::OP_dl[] PROGMEM = { "dl" };
const char firestep::OP_dp[] PROGMEM = { "dp" };
const char firestep::OP_dpy[] PROGMEM = { "dpy" };
const char firestep::OP_dpycb[] PROGMEM = { "dpycb" };
const char firestep::OP_dpycg[] PROGMEM = { "dpycg" };
const char firestep::OP_dpycr[] PROGMEM = { "dpycr" };
const char firestep::OP_dpydl[] PROGMEM = { "dpydl" };
const char firestep::OP_dpyds[] PROGMEM = { "dpyds" };
const char firestep::OP_ds[] PROGMEM = { "ds" };
const char firestep::OP_dvs[] PROGMEM = { "dvs" };
const char firestep::OP_e[] PROGMEM = { "e" };
const char firestep::OP_eep[] PROGMEM = { "eep" };
const char firestep::OP_en[] PROGMEM = { "en" };
const char firestep::OP_eu[] PROGMEM = { "eu" };
const char firestep::OP_f[] PROGMEM = { "f" };
const char firestep::OP_fpd_hex_probe[] PROGMEM = { "fpd-hex-probe" };
const char firestep::OP_fr[] PROGMEM = { "fr" };
const char firestep::OP_ge[] PROGMEM = { "ge" };
const char firestep::OP_gr1[] PROGMEM = { "gr1" };
const char firestep::OP_gr2[] PROGMEM = { "gr2" };
const char firestep::OP_gr3[] PROGMEM = { "gr3" };
const char firestep::OP_gr[] PROGMEM = { "gr" };
const char firestep::OP_ha1[] PROGMEM = { "ha1" };
const char firestep::OP_ha2[] PROGMEM = { "ha2" };
const char firestep::OP_ha3[] PROGMEM = { "ha3" };
const char firestep::OP_ha[] PROGMEM = { "ha" };
const char firestep::OP_he[] PROGMEM = { "he" };
const char firestep::OP_help[] PROGMEM = { "help" };
const char firestep::OP_ho[] PROGMEM = { "ho" };
const char firestep::OP_hom[] PROGMEM = { "hom" };
const char firestep::OP_hp[] PROGMEM = { "hp" };
const char firestep::OP_idl[] PROGMEM = { "idl" };
const char firestep::OP_io[] PROGMEM = { "io" };
const char firestep::OP_ip[] PROGMEM = { "ip" };
const char firestep::OP_is[] PROGMEM = { "is" };
const char firestep::OP_jp[] PROGMEM = { "jp" };
const char firestep::OP_lb[] PROGMEM = { "lb" };
const char firestep::OP_lh[] PROGMEM = { "lh" };
const char firestep::OP_lm[] PROGMEM = { "lm" };
const char firestep::OP_ln[] PROGMEM = { "ln" };
const char firestep::OP_lp[] PROGMEM = { "lp" };
const char firestep::OP_m1[] PROGMEM = { "m1" };
const char firestep::OP_m2[] PROGMEM = { "m2" };
const char firestep::OP_m3[] PROGMEM = { "m3" };
const char firestep::OP_m4[] PROGMEM = { "m4" };
const char firestep::OP_m5[] PROGMEM = { "m5" };
const char firestep::OP_m6[] PROGMEM = { "m6" };
const char firestep::OP_m7[] PROGMEM = { "m7" };
const char firestep::OP_m8[] PROGMEM = { "m8" };
const char firestep::OP_m9[] PROGMEM = { "m9" };
const char firestep::OP_ma[] PROGMEM = { "ma" };
const char firestep::OP_mi[] PROGMEM = { "mi" };
const char firestep::OP_mov[] PROGMEM = { "mov" };
const char firestep::OP_mova1[] PROGMEM = { "mova1" };
const char firestep::OP_mova2[] PROGMEM = { "mova2" };
const char firestep::OP_mova3[] PROGMEM = { "mova3" };
const char firestep::OP_movangle[] PROGMEM = { "movangle" };
const char firestep::OP_movrx[] PROGMEM = { "movrx" };
const char firestep::OP_movry[] PROGMEM = { "movry" };
const char firestep::OP_movrz[] PROGMEM = { "movrz" };
const char firestep::OP_movwp[] PROGMEM = { "movwp" };
const char firestep::OP_movxm[] PROGMEM = { "movxm" };
const char firestep::OP_movxr[] PROGMEM = { "movxr" };
const char firestep::OP_movym[] PROGMEM = { "movym" };
const char firestep::OP_movyr[] PROGMEM = { "movyr" };
const char firestep::OP_movzb[] PROGMEM = { "movzb" };
const char firestep::OP_movzm[] PROGMEM = { "movzm" };
const char firestep::OP_movzr[] PROGMEM = { "movzr" };
const char firestep::OP_mrk[] PROGMEM = { "mrk" };
const char firestep::OP_mrka1[] PROGMEM = { "mrka1" };
const char firestep::OP_mrka2[] PROGMEM = { "mrka2" };
const char firestep::OP_mrka3[] PROGMEM = { "mrka3" };
const char firestep::OP_mrkax[] PROGMEM = { "mrkax" };
const char firestep::OP_mrkay[] PROGMEM = { "mrkay" };
const char firestep::OP_mrkaz[] PROGMEM = { "mrkaz" };
const char firestep::OP_mrkm1[] PROGMEM = { "mrkm1" };
const char firestep::OP_mrkm2[] PROGMEM = { "mrkm2" };
const char firestep::OP_mrkm3[] PROGMEM = { "mrkm3" };
const char firestep::OP_mrkm4[] PROGMEM = { "mrkm4" };
const char firestep::OP_mrkm5[] PROGMEM = { "mrkm5" };
const char firestep::OP_mrkm6[] PROGMEM = { "mrkm6" };
const char firestep::OP_mrkm7[] PROGMEM = { "mrkm7" };
const char firestep::OP_mrkm8[] PROGMEM = { "mrkm8" };
const char firestep::OP_mrkm9[] PROGMEM = { "mrkm9" };
const char firestep::OP_mrkwp[] PROGMEM = { "mrkwp" };
const char firestep::OP_msg[] PROGMEM = { "msg" };
const char firestep::OP_mv[] PROGMEM = { "mv" };
const char firestep::OP_om[] PROGMEM = { "om" };
const char firestep::OP_pb[] PROGMEM = { "pb" };
const char firestep::OP_pc[] PROGMEM = { "pc" };
const char firestep::OP_pd[] PROGMEM = { "pd" };
const char firestep::OP_pe[] PROGMEM = { "pe" };
const char firestep::OP_pgm[] PROGMEM = { "pgm" };
const char firestep::OP_pgmd[] PROGMEM = { "pgmd" };
const char firestep::OP_pgmx[] PROGMEM = { "pgmx" };
const char firestep::OP_ph[] PROGMEM = { "ph" };
const char firestep::OP_pi[] PROGMEM = { "pi" };
const char firestep::OP_pm[] PROGMEM = { "pm" };
const char firestep::OP_pn[] PROGMEM = { "pn" };
const char firestep::OP_po[] PROGMEM = { "po" };
const char firestep::OP_pp[] PROGMEM = { "pp" };
const char firestep::OP_prb[] PROGMEM = { "prb" };
const char firestep::OP_prbd[] PROGMEM = { "prbd" };
const char firestep::OP_prbip[] PROGMEM = { "prbip" };
const char firestep::OP_prbpb[] PROGMEM = { "prbpb" };
const char firestep::OP_prbpn[] PROGMEM = { "prbpn" };
const char firestep::OP_prbsd[] PROGMEM = { "prbsd" };
const char firestep::OP_prbx[] PROGMEM = { "prbx" };
const char firestep::OP_prby[] PROGMEM = { "prby" };
const char firestep::OP_prbz[] PROGMEM = { "prbz" };
const char firestep::OP_prx[] PROGMEM = { "prx" };
const char firestep::OP_ps[] PROGMEM = { "ps" };
const char firestep::OP_pu[] PROGMEM = { "pu" };
const char firestep::OP_r[] PROGMEM = { "r" };
const char firestep::OP_re[] PROGMEM = { "re" };
const char firestep::OP_rf[] PROGMEM = { "rf" };
const char firestep::OP_rv[] PROGMEM = { "rv" };
const char firestep::OP_rx[] PROGMEM = { "rx" };
const char firestep::OP_ry[] PROGMEM = { "ry" };
const char firestep::OP_rz[] PROGMEM = { "rz" };
const char firestep::OP_sa[] PROGMEM = { "sa" };
const char firestep::OP_sc[] PROGMEM = { "sc" };
const char firestep::OP_sd[] PROGMEM = { "sd" };
const char firestep::OP_sg[] PROGMEM = { "sg" };
const char firestep::OP_sp[] PROGMEM = { "sp" };
const char firestep::OP_spa[] PROGMEM = { "spa" };
const char firestep::OP_spr[] PROGMEM = { "spr" };
const char firestep::OP_st[] PROGMEM = { "st" };
const char firestep::OP_sv[] PROGMEM = { "sv" };
const char firestep::OP_sys[] PROGMEM = { "sys" };
const char firestep::OP_sysah[] PROGMEM = { "sysah" };
const char firestep::OP_sysas[] PROGMEM = { "sysas" };
const char firestep::OP_sysch[] PROGMEM = { "sysch" };
const char firestep::OP_sysdb[] PROGMEM = { "sysdb" };
const char firestep::OP_syseu[] PROGMEM = { "syseu" };
const char firestep::OP_sysfr[] PROGMEM = { "sysfr" };
const char firestep::OP_syshp[] PROGMEM = { "syshp" };
const char firestep::OP_sysjp[] PROGMEM = { "sysjp" };
const char firestep::OP_syslh[] PROGMEM = { "syslh" };
const char firestep::OP_syslp[] PROGMEM = { "syslp" };
const char firestep::OP_sysmv[] PROGMEM = { "sysmv" };
const char firestep::OP_sysom[] PROGMEM = { "sysom" };
const char firestep::OP_syspb[] PROGMEM = { "syspb" };
const char firestep::OP_syspc[] PROGMEM = { "syspc" };
const char firestep::OP_syspi[] PROGMEM = { "syspi" };
const char firestep::OP_syssd[] PROGMEM = { "syssd" };
const char firestep::OP_systc[] PROGMEM = { "systc" };
const char firestep::OP_systo[] PROGMEM = { "systo" };
const char firestep::OP_systv[] PROGMEM = { "systv" };
const char firestep::OP_sysv[] PROGMEM = { "sysv" };
const char firestep::OP_tc[] PROGMEM = { "tc" };
const char firestep::OP_test1[] PROGMEM = { "test1" };
const char firestep::OP_test2[] PROGMEM = { "test2" };
const char firestep::OP_test[] PROGMEM = { "test" };
const char firestep::OP_tm[] PROGMEM = { "tm" };
const char firestep::OP_tn[] PROGMEM = { "tn" };
const char firestep::OP_to[] PROGMEM = { "to" };
const char firestep::OP_tp[] PROGMEM = { "tp" };
const char firestep::OP_ts[] PROGMEM = { "ts" };
const char firestep::OP_tst[] PROGMEM = { "tst" };
const char firestep::OP_tstph[] PROGMEM = { "tstph" };
const char firestep::OP_tstrv[] PROGMEM = { "tstrv" };
const char firestep::OP_tstsp[] PROGMEM = { "tstsp" };
const char firestep::OP_tv[] PROGMEM = { "tv" };
const char firestep::OP_ud[] PROGMEM = { "ud" };
const char firestep::OP_us[] PROGMEM = { "us" };
const char firestep::OP_v[] PROGMEM = { "v" };
const char firestep::OP_wp[] PROGMEM = { "wp" };
const char firestep::OP_x[] PROGMEM = { "x" };
const char firestep::OP_xm[] PROGMEM = { "xm" };
const char firestep::OP_xr[] PROGMEM = { "xr" };
const char firestep::OP_y[] PROGMEM = { "y" };
const char firestep::OP_ym[] PROGMEM = { "ym" };
const char firestep::OP_yr[] PROGMEM = { "yr" };
const char firestep::OP_z[] PROGMEM = { "z" };
const char firestep::OP_zb[] PROGMEM = { "zb" };
const char firestep::OP_zc[] PROGMEM = { "zc" };
const char firestep::OP_zm[] PROGMEM = { "zm" };
const char firestep::OP_zr[] PROGMEM = { "zr" };

const char src_help[] PROGMEM = {
    "["
    "{\"msg\":\"Program names are:\"},"
    "{\"msg\":\"  help             print this help text\"},"
    "{\"msg\":\"  test             print test message\"},"
    //"{\"msg\":\"  cal              Use hex probe to calibrate home angle, gear ratio and Z-bed plane (non-adaptive)\"}"
    //"{\"msg\":\"  cal-coarse       Use hex probe to calibrate home angle, gear ratio and Z-bed plane (adaptive coarse)\"}"
    //"{\"msg\":\"  cal-fine         Use hex probe to calibrate home angle, gear ratio and Z-bed plane (adaptive fine)\"}"
    //"{\"msg\":\"  cal-gear         Use hex probe to calibrate gear ratio and Z-bed plane (non-adaptive)\"}"
    //"{\"msg\":\"  cal-gear-coarse  Use hex probe to calibrate gear ratio and Z-bed plane (adaptive coarse)\"}"
    //"{\"msg\":\"  cal-gear-fine    Use hex probe to calibrate gear ratio and Z-bed plane (adaptive fine)\"}"
    "{\"msg\":\"  cal-fpd-home-coarse  Use hex probe to calibrate FPD home angle and Z-bed plane (adaptive coarse)\"}"
    "{\"msg\":\"  cal-fpd-home-medium  Use hex probe to calibrate FPD home angle and Z-bed plane (adaptive medium)\"}"
    "{\"msg\":\"  cal-fpd-home-fine    Use hex probe to calibrate FPD home angle and Z-bed plane (adaptive fine)\"}"
    "{\"msg\":\"  cal-fpd-bed-coarse   Use hex probe to calibrate FPD Z-bed plane (adaptive coarse)\"}"
    "{\"msg\":\"  cal-fpd-bed-medium   Use hex probe to calibrate FPD Z-bed plane (adaptive medium)\"}"
    "{\"msg\":\"  cal-fpd-bed-fine     Use hex probe to calibrate FPD Z-bed plane (adaptive fine)\"}"
    "{\"msg\":\"  fpd-hex-probe        Perform hex probe and return probe Z-data\"}"
    "]"
};

const char src_test1[] PROGMEM = {
    "{\"msg\":\"test A\"}"
};

const char src_test2[] PROGMEM = {
    "["
    "{\"msg\":\"test A\"},"
    "{\"msg\":\"test B\"}"
    "]"
};

#define HEX_PROBE \
    "{\"hom\":\"\"},"\
    "{\"prbz\":\"\"},"\
    "{\"movzr\":10},"\
    "{\"mrkwp\":1},"\
    "{\"mov\":{ \"zm\":3,\"angle\":0,\"d\":50}},"\
    "{\"prbz\":\"\"},"\
    "{\"mov\":{\"zm\":3,\"angle\":60,\"d\":50}},"\
    "{\"prbz\":\"\"},"\
    "{\"mov\":{\"zm\":3,\"angle\":120,\"d\":50}},"\
    "{\"prbz\":\"\"},"\
    "{\"mov\":{\"zm\":3,\"angle\":180,\"d\":50}},"\
    "{\"prbz\":\"\"},"\
    "{\"mov\":{\"zm\":3,\"angle\":240,\"d\":50}},"\
    "{\"prbz\":\"\"},"\
    "{\"mov\":{\"zm\":3,\"angle\":300,\"d\":50}},"\
    "{\"prbz\":\"\"},"\
    "{\"mov\":{\"zm\":3,\"x\":0,\"y\":0}},"\
    "{\"prbz\":\"\"}"

#define HEX_PROBE_CW \
    "{\"hom\":\"\"},"\
    "{\"prbz\":\"\"},"\
    "{\"movzr\":10},"\
    "{\"mrkwp\":1},"\
    "{\"mov\":{\"zm\":3,\"angle\":300,\"d\":50}},"\
    "{\"prbz\":\"\"},"\
    "{\"mov\":{\"zm\":3,\"angle\":240,\"d\":50}},"\
    "{\"prbz\":\"\"},"\
    "{\"mov\":{\"zm\":3,\"angle\":180,\"d\":50}},"\
    "{\"prbz\":\"\"},"\
    "{\"mov\":{\"zm\":3,\"angle\":120,\"d\":50}},"\
    "{\"prbz\":\"\"},"\
    "{\"mov\":{\"zm\":3,\"angle\":60,\"d\":50}},"\
    "{\"prbz\":\"\"},"\
    "{\"mov\":{ \"zm\":3,\"angle\":0,\"d\":50}},"\
    "{\"prbz\":\"\"},"\
    "{\"mov\":{\"zm\":3,\"x\":0,\"y\":0}},"\
    "{\"prbz\":\"\"}"

#define CAL_FPD_HOME_COARSE \
	HEX_PROBE "," \
    "{\"cal\":{\"bz\":\"\",\"ha\":\"\",\"sv\":1.0,\"zr\":\"\",\"zc\":\"\"}}," \
    "{\"dim\":{\"bx\":\"\",\"by\":\"\",\"bz\":\"\"},\"hom\":\"\"}"

#define CAL_FPD_HOME_MEDIUM \
	HEX_PROBE "," \
    "{\"cal\":{\"bz\":\"\",\"ha\":\"\",\"sv\":0.7,\"zr\":\"\",\"zc\":\"\"}}," \
    "{\"dim\":{\"bx\":\"\",\"by\":\"\",\"bz\":\"\"},\"hom\":\"\"}"

#define CAL_FPD_HOME_FINE \
	HEX_PROBE "," \
    "{\"cal\":{\"bz\":\"\",\"ha\":\"\",\"sv\":0.3,\"zr\":\"\",\"zc\":\"\"}}," \
    "{\"dim\":{\"bx\":\"\",\"by\":\"\",\"bz\":\"\"},\"hom\":\"\"}"

const char src_calibrate[] PROGMEM = {
    "[" HEX_PROBE ","
    "{\"cal\":{\"bx\":\"\",\"by\":\"\",\"bz\":\"\",\"gr\":\"\",\"ge\":\"\",\"ha\":\"\",\"he\":\"\",\"sv\":1.0,\"zr\":\"\",\"zc\":\"\"}}"
    "]"
};

const char src_cal_fine[] PROGMEM = {
    "[" HEX_PROBE ","
    "{\"cal\":{\"bx\":\"\",\"by\":\"\",\"bz\":\"\",\"gr\":\"\",\"ge\":\"\",\"ha\":\"\",\"he\":\"\",\"sv\":0.3,\"zr\":\"\",\"zc\":\"\"}}"
    "]"
};

const char src_cal_coarse[] PROGMEM = {
    "[" HEX_PROBE ","
    "{\"cal\":{\"bx\":\"\",\"by\":\"\",\"bz\":\"\",\"gr\":\"\",\"ge\":\"\",\"ha\":\"\",\"he\":\"\",\"sv\":0.7,\"zr\":\"\",\"zc\":\"\"}}"
    "]"
};

const char src_cal_fpd_bed_fine[] PROGMEM = {
    "[" HEX_PROBE ","
    "{\"cal\":{\"bx\":\"\",\"by\":\"\",\"bz\":\"\",\"bx\":\"\",\"by\":\"\",\"bz\":\"\",\"he\":\"\",\"sv\":0.3,\"zr\":\"\",\"zc\":\"\"}}"
    "]"
};

const char src_cal_fpd_bed_medium[] PROGMEM = {
    "[" HEX_PROBE ","
    "{\"cal\":{\"bx\":\"\",\"by\":\"\",\"bz\":\"\",\"bx\":\"\",\"by\":\"\",\"bz\":\"\",\"he\":\"\",\"sv\":0.7,\"zr\":\"\",\"zc\":\"\"}}"
    "]"
};

const char src_cal_fpd_bed_coarse[] PROGMEM = {
    "[" HEX_PROBE ","
    "{\"cal\":{\"bx\":\"\",\"by\":\"\",\"bz\":\"\",\"bx\":\"\",\"by\":\"\",\"bz\":\"\",\"he\":\"\",\"sv\":1.0,\"zr\":\"\",\"zc\":\"\"}}"
    "]"
};

const char src_cal_fpd_home_medium[] PROGMEM = {
    "[" CAL_FPD_HOME_MEDIUM "]"
};

const char src_cal_fpd_home_coarse[] PROGMEM = {
    "[" CAL_FPD_HOME_COARSE "]"
};

const char src_cal_fpd_home_fine[] PROGMEM = {
    "[" CAL_FPD_HOME_FINE "]"
};

const char src_cal_gear[] PROGMEM = {
    "[" HEX_PROBE ","
    "{\"cal\":{\"bx\":\"\",\"by\":\"\",\"bz\":\"\",\"gr\":\"\",\"ge\":\"\",\"sv\":1.0,\"zr\":\"\",\"zc\":\"\"}}"
    "]"
};

const char src_cal_gear_coarse[] PROGMEM = {
    "[" HEX_PROBE ","
    "{\"cal\":{\"bx\":\"\",\"by\":\"\",\"bz\":\"\",\"gr\":\"\",\"ge\":\"\",\"sv\":0.7,\"zr\":\"\",\"zc\":\"\"}}"
    "]"
};

const char src_cal_gear_fine[] PROGMEM = {
    "[" HEX_PROBE ","
    "{\"cal\":{\"bx\":\"\",\"by\":\"\",\"bz\":\"\",\"gr\":\"\",\"ge\":\"\",\"sv\":0.3,\"zr\":\"\",\"zc\":\"\"}}"
    "]"
};

const char src_fpd_hex_probe[] PROGMEM = {
    "[" HEX_PROBE "]"
};

const char src_dim_fpd[] PROGMEM = {
    "["
    "{\"dim\":{"
    "\"gr\":" FPD_GEAR_RATIO_S "," // gear ratio
    "\"ha\":-67.2," // home angle
    "\"e\":131.636," // effector triangle side
    "\"f\":190.526," // base triangle side
    "\"re\":270.000," // effector arm length (mm)
    "\"rf\":90.000," // pulley arm length (mm)
    "\"spa\":" FPD_SPE_ANGLE_S "," // MC:arm critical angle https://github.com/firepick1/FireStep/wiki/Sliced-Pulley-Error
    "\"spr\":" FPD_SPE_RATIO_S "}}" // SPE Ratio https://github.com/firepick1/FireStep/wiki/Sliced-Pulley-Error
    "]"
};

const char *firestep::prog_src(const char *name) {
    if (false) { // must be sorted reverse alphabetical

    } else if (strcmp_PS(OP_test, name) == 0) {
        return src_test2;
        //} else if (strcmp_PS(OP_cal, name) == 0) { return src_calibrate;
        //} else if (strcmp_PS(OP_cal_coarse, name) == 0) { return src_cal_coarse;
        //} else if (strcmp_PS(OP_cal_fine, name) == 0) { return src_cal_fine;
        //} else if (strcmp_PS(OP_cal_gear, name) == 0) { return src_cal_gear;
        //} else if (strcmp_PS(OP_cal_gear_coarse, name) == 0) { return src_cal_gear_coarse;
        //} else if (strcmp_PS(OP_cal_gear_fine, name) == 0) { return src_cal_gear_fine;
    } else if (strcmp_PS(OP_cal_fpd_bed_coarse, name) == 0) {
        return src_cal_fpd_bed_coarse;
    } else if (strcmp_PS(OP_cal_fpd_bed_fine, name) == 0) {
        return src_cal_fpd_bed_fine;
    } else if (strcmp_PS(OP_cal_fpd_bed_medium, name) == 0) {
        return src_cal_fpd_bed_medium;
    } else if (strcmp_PS(OP_cal_fpd_home_coarse, name) == 0) {
        return src_cal_fpd_home_coarse;
    } else if (strcmp_PS(OP_cal_fpd_home_fine, name) == 0) {
        return src_cal_fpd_home_fine;
    } else if (strcmp_PS(OP_cal_fpd_home_medium, name) == 0) {
        return src_cal_fpd_home_medium;
    } else if (strcmp_PS(OP_dim_fpd, name) == 0) {
        return src_dim_fpd;
    } else if (strcmp_PS(OP_fpd_hex_probe, name) == 0) {
        return src_fpd_hex_probe;
    } else if (strcmp_PS(OP_help, name) == 0) {
        return src_help;
    } else if (strcmp_PS(OP_test1, name) == 0) {
        return src_test1;
    } else if (strcmp_PS(OP_test2, name) == 0) {
        return src_test2;

    }

    return src_help;
}

Status firestep::prog_dump(const char *name) {
    const char *src = prog_src(name);
    TESTCOUT3("prog_dump:", name, " bytes:", (strlen_P(src)+1), " src:", src);

    for (size_t i = 0; i<MAX_JSON; i++) {
        char c = pgm_read_byte_near(src + i);
        ASSERT(c == 0 || ' ' <= c && c <= '~');
        if (c) {
            Serial.print(c);
        } else {
            Serial.println();
            break;
        }
    }

    return STATUS_OK;
}

Status firestep::prog_load_cmd(const char *name, JsonCommand &jcmd) {
    char nameBuf[32];
    snprintf(nameBuf, sizeof(nameBuf), "%s", name); // name is volatile
    Status status = STATUS_OK;
    const char *src = prog_src(nameBuf);
    TESTCOUT2("prog_load:", nameBuf, " src:", src);

    size_t len = strlen_P(src);
    if (len <= 0 || MAX_JSON <= len+1) {
        return STATUS_PROGRAM_SIZE;
    }

    ///////// DANGER /////////
    // We will replace the currently running command with the program
    // name will no longer be valid
    jcmd.clear();
    ///////// DANGER /////////

    char *buf = jcmd.allocate(len+1);
    ASSERT(buf);
    for (size_t i = 0; i < len; i++) {
        buf[i] = pgm_read_byte_near(src + i);
        ASSERT(' ' <= buf[i] && buf[i] <= '~');
    }
    buf[len] = 0;
    TESTCOUT3("prog_load_cmd:", nameBuf, " buf:", buf, " status:", status);
    if (status != STATUS_OK) {
        TESTCOUT1("prog_load_cmd:", status);
        return status;
    }

    status = jcmd.parse(buf, STATUS_WAIT_IDLE);
    if (status < 0) {
        TESTCOUT2("prog_load_cmd:", nameBuf, " parse error:", status); // should never happen
    } else {
        TESTCOUT2("prog_load_cmd:", nameBuf, " parse status:", status); // STATUS_BUSY_PARSED 10
    }

    return status;
}

