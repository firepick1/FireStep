#include "ProgMem.h"

using namespace firestep;

const char firestep::OP_1[] PROGMEM = { "1" };
const char firestep::OP_2[] PROGMEM = { "2" };
const char firestep::OP_3[] PROGMEM = { "3" };
const char firestep::OP_4[] PROGMEM = { "4" };
const char firestep::OP_a[] PROGMEM = { "a" };
const char firestep::OP_ah[] PROGMEM = { "ah" };
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
const char firestep::OP_calbx[] PROGMEM = { "calbx" };
const char firestep::OP_calby[] PROGMEM = { "calby" };
const char firestep::OP_calbz[] PROGMEM = { "calbz" };
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
const char firestep::OP_dimbx[] PROGMEM = { "dimbx" };
const char firestep::OP_dimby[] PROGMEM = { "dimby" };
const char firestep::OP_dimbz[] PROGMEM = { "dimbz" };
const char firestep::OP_dime[] PROGMEM = { "dime" };
const char firestep::OP_dimf[] PROGMEM = { "dimf" };
const char firestep::OP_dimgr[] PROGMEM = { "dimgr" };
const char firestep::OP_dimha[] PROGMEM = { "dimha" };
const char firestep::OP_dimha1[] PROGMEM = { "dimha1" };
const char firestep::OP_dimha2[] PROGMEM = { "dimha2" };
const char firestep::OP_dimha3[] PROGMEM = { "dimha3" };
const char firestep::OP_dimhp[] PROGMEM = { "dimhp" };
const char firestep::OP_dimmi[] PROGMEM = { "dimmi" };
const char firestep::OP_dimpd[] PROGMEM = { "dimpd" };
const char firestep::OP_dimre[] PROGMEM = { "dimre" };
const char firestep::OP_dimrf[] PROGMEM = { "dimrf" };
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
const char firestep::OP_fr[] PROGMEM = { "fr" };
const char firestep::OP_gr[] PROGMEM = { "gr" };
const char firestep::OP_ha[] PROGMEM = { "ha" };
const char firestep::OP_ha1[] PROGMEM = { "ha1" };
const char firestep::OP_ha2[] PROGMEM = { "ha2" };
const char firestep::OP_ha3[] PROGMEM = { "ha3" };
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
const char firestep::OP_syspc[] PROGMEM = { "syspc" };
const char firestep::OP_syspi[] PROGMEM = { "syspi" };
const char firestep::OP_syssd[] PROGMEM = { "syssd" };
const char firestep::OP_systc[] PROGMEM = { "systc" };
const char firestep::OP_systo[] PROGMEM = { "systo" };
const char firestep::OP_systv[] PROGMEM = { "systv" };
const char firestep::OP_sysv[] PROGMEM = { "sysv" };
const char firestep::OP_tc[] PROGMEM = { "tc" };
const char firestep::OP_test[] PROGMEM = { "test" };
const char firestep::OP_test1[] PROGMEM = { "test1" };
const char firestep::OP_test2[] PROGMEM = { "test2" };
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
    "{\"msg\":\"  help  print this help text\"},"
    "{\"msg\":\"  test  print test message\"},"
    "{\"msg\":\"  cal   calibrate Z-bowl error and Z-bed plane\"}"
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

const char src_calibrate[] PROGMEM = {
    "["
    "{\"hom\":\"\"},"
    "{\"prbz\":\"\"},"
    "{\"movzr\":10},"
    "{\"mrkwp\":1},"
    "{\"mov\":{ \"zm\":3,\"a\":0,\"d\":50}},"
    "{\"prbz\":\"\"},"
    "{\"mov\":{\"zm\":3,\"a\":60,\"d\":50}},"
    "{\"prbz\":\"\"},"
    "{\"mov\":{\"zm\":3,\"a\":120,\"d\":50}},"
    "{\"prbz\":\"\"},"
    "{\"mov\":{\"zm\":3,\"a\":180,\"d\":50}},"
    "{\"prbz\":\"\"},"
    "{\"mov\":{\"zm\":3,\"a\":240,\"d\":50}},"
    "{\"prbz\":\"\"},"
    "{\"mov\":{\"zm\":3,\"a\":300,\"d\":50}},"
    "{\"prbz\":\"\"},"
    "{\"mov\":{\"zm\":3,\"x\":0,\"y\":0}},"
    "{\"prbz\":\"\"},"
    "{\"movwp\":1},"
    "{\"prbd\":\"\",\"cal\":\"\"}"
    "]"
};

const char *firestep::prog_src(const char *name) {
    if (strcmp_P(OP_test, name) == 0) {
        return src_test2;
    } else if (strcmp_P(OP_test1, name) == 0) {
        return src_test1;
    } else if (strcmp_P(OP_test2, name) == 0) {
        return src_test2;
    } else if (strcmp_P(OP_help, name) == 0) {
        return src_help;
    } else if (strcmp_P(OP_cal, name) == 0) {
        return src_calibrate;
    }

    return src_help;
}

Status firestep::prog_dump(const char *name) {
    const char *src = prog_src(name);
    TESTCOUT2("prog_dump:", name, " src:", src);

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

