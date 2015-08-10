#include "ProgMem.h"

#ifndef ARDUINO
#define PROGMEM
#define strlen_P(src) strlen(src)
#define pgm_read_byte_near(src) (*(const char *)(src))
#endif

using namespace firestep;

const char OP_1[] PROGMEM = { "1" };
const char OP_2[] PROGMEM = { "2" };
const char OP_3[] PROGMEM = { "3" };
const char OP_4[] PROGMEM = { "4" };
const char OP_a[] PROGMEM = { "a" };
const char OP_ah[] PROGMEM = { "ah" };
const char OP_as[] PROGMEM = { "as" };
const char OP_ax[] PROGMEM = { "ax" };
const char OP_ay[] PROGMEM = { "ay" };
const char OP_az[] PROGMEM = { "az" };
const char OP_b[] PROGMEM = { "b" };
const char OP_bx[] PROGMEM = { "bx" };
const char OP_by[] PROGMEM = { "by" };
const char OP_bz[] PROGMEM = { "bz" };
const char OP_c[] PROGMEM = { "c" };
const char OP_cal[] PROGMEM = { "cal" };
const char OP_calbx[] PROGMEM = { "calbx" };
const char OP_calby[] PROGMEM = { "calby" };
const char OP_calbz[] PROGMEM = { "calbz" };
const char OP_calha[] PROGMEM = { "calha" };
const char OP_calhe[] PROGMEM = { "calhe" };
const char OP_calsv[] PROGMEM = { "calsv" };
const char OP_calzc[] PROGMEM = { "calzc" };
const char OP_calzr[] PROGMEM = { "calzr" };
const char OP_cb[] PROGMEM = { "cb" };
const char OP_cg[] PROGMEM = { "cg" };
const char OP_ch[] PROGMEM = { "ch" };
const char OP_cmt[] PROGMEM = { "cmt" };
const char OP_cr[] PROGMEM = { "cr" };
const char OP_d[] PROGMEM = { "d" };
const char OP_db[] PROGMEM = { "db" };
const char OP_dh[] PROGMEM = { "dh" };
const char OP_dim[] PROGMEM = { "dim" };
const char OP_dimbx[] PROGMEM = { "dimbx" };
const char OP_dimby[] PROGMEM = { "dimby" };
const char OP_dimbz[] PROGMEM = { "dimbz" };
const char OP_dime[] PROGMEM = { "dime" };
const char OP_dimf[] PROGMEM = { "dimf" };
const char OP_dimgr[] PROGMEM = { "dimgr" };
const char OP_dimha[] PROGMEM = { "dimha" };
const char OP_dimha1[] PROGMEM = { "dimha1" };
const char OP_dimha2[] PROGMEM = { "dimha2" };
const char OP_dimha3[] PROGMEM = { "dimha3" };
const char OP_dimmi[] PROGMEM = { "dimmi" };
const char OP_dimpd[] PROGMEM = { "dimpd" };
const char OP_dimre[] PROGMEM = { "dimre" };
const char OP_dimrf[] PROGMEM = { "dimrf" };
const char OP_dimst[] PROGMEM = { "dimst" };
const char OP_dl[] PROGMEM = { "dl" };
const char OP_dp[] PROGMEM = { "dp" };
const char OP_dpy[] PROGMEM = { "dpy" };
const char OP_dpycb[] PROGMEM = { "dpycb" };
const char OP_dpycg[] PROGMEM = { "dpycg" };
const char OP_dpycr[] PROGMEM = { "dpycr" };
const char OP_dpydl[] PROGMEM = { "dpydl" };
const char OP_dpyds[] PROGMEM = { "dpyds" };
const char OP_ds[] PROGMEM = { "ds" };
const char OP_dvs[] PROGMEM = { "dvs" };
const char OP_e[] PROGMEM = { "e" };
const char OP_eep[] PROGMEM = { "eep" };
const char OP_en[] PROGMEM = { "en" };
const char OP_eu[] PROGMEM = { "eu" };
const char OP_f[] PROGMEM = { "f" };
const char OP_fr[] PROGMEM = { "fr" };
const char OP_gr[] PROGMEM = { "gr" };
const char OP_ha[] PROGMEM = { "ha" };
const char OP_ha1[] PROGMEM = { "ha1" };
const char OP_ha2[] PROGMEM = { "ha2" };
const char OP_ha3[] PROGMEM = { "ha3" };
const char OP_he[] PROGMEM = { "he" };
const char OP_help[] PROGMEM = { "help" };
const char OP_ho[] PROGMEM = { "ho" };
const char OP_hom[] PROGMEM = { "hom" };
const char OP_hp[] PROGMEM = { "hp" };
const char OP_idl[] PROGMEM = { "idl" };
const char OP_io[] PROGMEM = { "io" };
const char OP_ip[] PROGMEM = { "ip" };
const char OP_is[] PROGMEM = { "is" };
const char OP_jp[] PROGMEM = { "jp" };
const char OP_lb[] PROGMEM = { "lb" };
const char OP_lh[] PROGMEM = { "lh" };
const char OP_lm[] PROGMEM = { "lm" };
const char OP_ln[] PROGMEM = { "ln" };
const char OP_lp[] PROGMEM = { "lp" };
const char OP_m1[] PROGMEM = { "m1" };
const char OP_m2[] PROGMEM = { "m2" };
const char OP_m3[] PROGMEM = { "m3" };
const char OP_m4[] PROGMEM = { "m4" };
const char OP_m5[] PROGMEM = { "m5" };
const char OP_m6[] PROGMEM = { "m6" };
const char OP_m7[] PROGMEM = { "m7" };
const char OP_m8[] PROGMEM = { "m8" };
const char OP_m9[] PROGMEM = { "m9" };
const char OP_ma[] PROGMEM = { "ma" };
const char OP_mi[] PROGMEM = { "mi" };
const char OP_mov[] PROGMEM = { "mov" };
const char OP_movrx[] PROGMEM = { "movrx" };
const char OP_movry[] PROGMEM = { "movry" };
const char OP_movrz[] PROGMEM = { "movrz" };
const char OP_movwp[] PROGMEM = { "movwp" };
const char OP_movxm[] PROGMEM = { "movxm" };
const char OP_movxr[] PROGMEM = { "movxr" };
const char OP_movym[] PROGMEM = { "movym" };
const char OP_movyr[] PROGMEM = { "movyr" };
const char OP_movzb[] PROGMEM = { "movzb" };
const char OP_movzm[] PROGMEM = { "movzm" };
const char OP_movzr[] PROGMEM = { "movzr" };
const char OP_mrk[] PROGMEM = { "mrk" };
const char OP_mrkax[] PROGMEM = { "mrkax" };
const char OP_mrkay[] PROGMEM = { "mrkay" };
const char OP_mrkaz[] PROGMEM = { "mrkaz" };
const char OP_mrkm1[] PROGMEM = { "mrkm1" };
const char OP_mrkm2[] PROGMEM = { "mrkm2" };
const char OP_mrkm3[] PROGMEM = { "mrkm3" };
const char OP_mrkm4[] PROGMEM = { "mrkm4" };
const char OP_mrkm5[] PROGMEM = { "mrkm5" };
const char OP_mrkm6[] PROGMEM = { "mrkm6" };
const char OP_mrkm7[] PROGMEM = { "mrkm7" };
const char OP_mrkm8[] PROGMEM = { "mrkm8" };
const char OP_mrkm9[] PROGMEM = { "mrkm9" };
const char OP_mrkwp[] PROGMEM = { "mrkwp" };
const char OP_msg[] PROGMEM = { "msg" };
const char OP_mv[] PROGMEM = { "mv" };
const char OP_om[] PROGMEM = { "om" };
const char OP_pc[] PROGMEM = { "pc" };
const char OP_pd[] PROGMEM = { "pd" };
const char OP_pe[] PROGMEM = { "pe" };
const char OP_pgm[] PROGMEM = { "pgm" };
const char OP_pgmd[] PROGMEM = { "pgmd" };
const char OP_pgmx[] PROGMEM = { "pgmx" };
const char OP_ph[] PROGMEM = { "ph" };
const char OP_pi[] PROGMEM = { "pi" };
const char OP_pm[] PROGMEM = { "pm" };
const char OP_pn[] PROGMEM = { "pn" };
const char OP_po[] PROGMEM = { "po" };
const char OP_pp[] PROGMEM = { "pp" };
const char OP_prb[] PROGMEM = { "prb" };
const char OP_prbd[] PROGMEM = { "prbd" };
const char OP_prbip[] PROGMEM = { "prbip" };
const char OP_prbpn[] PROGMEM = { "prbpn" };
const char OP_prbsd[] PROGMEM = { "prbsd" };
const char OP_prbx[] PROGMEM = { "prbx" };
const char OP_prby[] PROGMEM = { "prby" };
const char OP_prbz[] PROGMEM = { "prbz" };
const char OP_prx[] PROGMEM = { "prx" };
const char OP_ps[] PROGMEM = { "ps" };
const char OP_pu[] PROGMEM = { "pu" };
const char OP_r[] PROGMEM = { "r" };
const char OP_re[] PROGMEM = { "re" };
const char OP_rf[] PROGMEM = { "rf" };
const char OP_rv[] PROGMEM = { "rv" };
const char OP_rx[] PROGMEM = { "rx" };
const char OP_ry[] PROGMEM = { "ry" };
const char OP_rz[] PROGMEM = { "rz" };
const char OP_sa[] PROGMEM = { "sa" };
const char OP_sc[] PROGMEM = { "sc" };
const char OP_sd[] PROGMEM = { "sd" };
const char OP_sg[] PROGMEM = { "sg" };
const char OP_sp[] PROGMEM = { "sp" };
const char OP_st[] PROGMEM = { "st" };
const char OP_sv[] PROGMEM = { "sv" };
const char OP_sys[] PROGMEM = { "sys" };
const char OP_sysah[] PROGMEM = { "sysah" };
const char OP_sysas[] PROGMEM = { "sysas" };
const char OP_sysch[] PROGMEM = { "sysch" };
const char OP_sysdb[] PROGMEM = { "sysdb" };
const char OP_syseu[] PROGMEM = { "syseu" };
const char OP_sysfr[] PROGMEM = { "sysfr" };
const char OP_syshp[] PROGMEM = { "syshp" };
const char OP_sysjp[] PROGMEM = { "sysjp" };
const char OP_syslh[] PROGMEM = { "syslh" };
const char OP_syslp[] PROGMEM = { "syslp" };
const char OP_sysmv[] PROGMEM = { "sysmv" };
const char OP_sysom[] PROGMEM = { "sysom" };
const char OP_syspc[] PROGMEM = { "syspc" };
const char OP_syspi[] PROGMEM = { "syspi" };
const char OP_syssd[] PROGMEM = { "syssd" };
const char OP_systc[] PROGMEM = { "systc" };
const char OP_systo[] PROGMEM = { "systo" };
const char OP_systv[] PROGMEM = { "systv" };
const char OP_sysv[] PROGMEM = { "sysv" };
const char OP_tc[] PROGMEM = { "tc" };
const char OP_test[] PROGMEM = { "test" };
const char OP_test1[] PROGMEM = { "test1" };
const char OP_test2[] PROGMEM = { "test2" };
const char OP_tm[] PROGMEM = { "tm" };
const char OP_tn[] PROGMEM = { "tn" };
const char OP_to[] PROGMEM = { "to" };
const char OP_tp[] PROGMEM = { "tp" };
const char OP_ts[] PROGMEM = { "ts" };
const char OP_tst[] PROGMEM = { "tst" };
const char OP_tstph[] PROGMEM = { "tstph" };
const char OP_tstrv[] PROGMEM = { "tstrv" };
const char OP_tstsp[] PROGMEM = { "tstsp" };
const char OP_tv[] PROGMEM = { "tv" };
const char OP_ud[] PROGMEM = { "ud" };
const char OP_us[] PROGMEM = { "us" };
const char OP_v[] PROGMEM = { "v" };
const char OP_wp[] PROGMEM = { "wp" };
const char OP_x[] PROGMEM = { "x" };
const char OP_xm[] PROGMEM = { "xm" };
const char OP_xr[] PROGMEM = { "xr" };
const char OP_y[] PROGMEM = { "y" };
const char OP_ym[] PROGMEM = { "ym" };
const char OP_yr[] PROGMEM = { "yr" };
const char OP_z[] PROGMEM = { "z" };
const char OP_zb[] PROGMEM = { "zb" };
const char OP_zc[] PROGMEM = { "zc" };
const char OP_zm[] PROGMEM = { "zm" };
const char OP_zr[] PROGMEM = { "zr" };

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
    if (strcmp("test", name) == 0) {
        return src_test2;
    } else if (strcmp("test1", name) == 0) {
        return src_test1;
    } else if (strcmp("test2", name) == 0) {
        return src_test2;
    } else if (strcmp("help", name) == 0) {
        return src_help;
    } else if (strcmp("cal", name) == 0) {
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

