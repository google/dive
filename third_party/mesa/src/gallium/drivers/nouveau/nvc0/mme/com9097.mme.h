const uint32_t mme9097_per_instance_bf[] = {
/* 0x0003: loop */
	0x00000301,
	0x00000211,
	0x05880021,
	0xffffc911,
	0x0040d043,
	0xffff8897,
	0x00005211,
};

const uint32_t mme9097_blend_enables[] = {
	0x05360021,
	0x00404042,
	0x00424042,
	0x00444042,
	0x00464042,
	0x00484042,
	0x004a4042,
	0x004c40c2,
	0x004e4042,
};

const uint32_t mme9097_vertex_array_select[] = {
	0x00000201,
	0x00000301,
	0x00000401,
	0x00000501,
	0x11004612,
	0x09004712,
	0x05c07621,
	0x00002041,
	0x00002841,
	0x05f03f21,
	0x000010c1,
	0x00001841,
};

const uint32_t mme9097_poly_mode_front[] = {
	0x00db0215,
	0x020c0315,
/* 0x000a: locn_0a_pmf */
	0x00128f10,
/* 0x000f: locn_0f_pmf */
	0x02100415,
	0x00004211,
	0x00180611,
	0x0014bf10,
	0x0000f807,
	0x00dac021,
	0x00800611,
	0x00131f10,
	0x0014bf10,
	0x0000f807,
	0x00000841,
	0x00000611,
	0x002ec0a1,
	0x00003041,
};

const uint32_t mme9097_poly_mode_back[] = {
	0x00dac215,
	0x020c0315,
/* 0x000a: locn_0a_pmb */
	0x00128f10,
/* 0x000f: locn_0f_pmb */
	0x02100415,
	0x00004211,
	0x00180611,
	0x0014bf10,
	0x0000f807,
	0x00db0021,
	0x00800611,
	0x00131f10,
	0x0014bf10,
	0x0000f807,
	0x00000841,
	0x00000611,
	0x002ec0a1,
	0x00003041,
};

const uint32_t mme9097_gp_select[] = {
	0x00dac215,
	0x00db0315,
/* 0x000a: locn_0a_gs */
	0x0012d710,
/* 0x000f: locn_0f_gs */
	0x020c0415,
	0x00004211,
	0x00180611,
	0x0014bf10,
	0x0000f807,
	0x02100021,
	0x00800611,
	0x00130f10,
	0x0014bf10,
	0x0000f807,
	0x00000841,
	0x00000611,
	0x002ec0a1,
	0x00003041,
};

const uint32_t mme9097_tep_select[] = {
	0x00dac215,
	0x00db0315,
/* 0x000a: locn_0a_ts */
	0x0012d710,
/* 0x000f: locn_0f_ts */
	0x02100415,
	0x00004211,
	0x00180611,
	0x0014bf10,
	0x0000f807,
	0x020c0021,
	0x00800611,
	0x00130f10,
	0x0014bf10,
	0x0000f807,
	0x00000841,
	0x00000611,
	0x002ec0a1,
	0x00003041,
};

const uint32_t mme9097_draw_elts_indirect[] = {
	0x01434615,
/* 0x0007: dei_draw_again */
	0x01438715,
	0x07400021,
	0x00003041,
	0x00003841,
	0x00000601,
/* 0x0018: dei_again */
	0x00000701,
	0x00000301,
/* 0x0020: dei_end */
	0x00000201,
	0x017dc451,
	0x00002431,
	0x0638c021,
	0x00680041,
	0x0004d007,
	0x00002531,
	0x00002841,
	0x00003041,
	0x05434021,
	0x00002041,
	0x00002841,
	0x01118021,
	0x00002041,
	0x00004411,
	0xd0400912,
	0x01618021,
	0x00000841,
	0x017e0021,
	0x00001841,
	0x00051210,
	0x01614071,
	0xfffe9017,
	0xd0410912,
	0xffffff11,
	0xfff9b817,
	0x00007611,
	0x03400615,
	0x03404715,
	0x05434021,
	0x00003041,
	0x00003841,
	0x011180a1,
	0x00003041,
};

const uint32_t mme9097_draw_arrays_indirect[] = {
/* 0x0003: dai_draw_again */
	0x01438515,
	0x00000601,
	0x00000701,
	0x00000201,
/* 0x0011: dai_again */
	0x00000301,
	0x00d74451,
/* 0x0019: dai_end */
	0x0004d807,
	0x00002431,
	0x0638c021,
	0x00680041,
	0x00000041,
	0x00002041,
	0x00003041,
	0x01438021,
	0x00002041,
	0x00004411,
	0xd0400912,
	0x01618021,
	0x00000841,
	0x00d78021,
	0x00001041,
	0x00051b10,
	0x01614071,
	0xfffe9817,
	0xd0410912,
	0xffffff11,
	0xfffa7817,
	0x00007611,
	0x014380a1,
	0x00002841,
};

const uint32_t mme9097_draw_elts_indirect_count[] = {
	0x01434615,
	0x01438715,
	0x07400021,
/* 0x000d: deic_draw_again */
	0x00003041,
	0x00003841,
	0x00000601,
	0x00000701,
/* 0x001e: deic_again */
	0x00000501,
	0x0005ad10,
/* 0x0026: deic_end */
	0x000b2807,
	0x007f4312,
/* 0x002e: deic_restore */
	0x000a9817,
	0x00002841,
/* 0x0035: deic_runout */
	0x00000301,
/* 0x003b: deic_runout_check */
	0x00000201,
	0x017dc451,
	0x00002431,
	0x0638c021,
	0x00680041,
	0x0004d007,
	0x00002531,
	0x00002841,
	0x00003041,
	0x05434021,
	0x00002041,
	0x00002841,
	0x01118021,
	0x00002041,
	0x00004411,
	0xd0400912,
	0x01618021,
	0x00000841,
	0x017e0021,
	0x00001841,
	0x00051210,
	0x01614071,
	0xfffe9017,
	0xd0410912,
	0x03408515,
	0xffffed11,
	0x0004e807,
	0xffffff11,
	0x03408021,
	0x00002841,
	0xfff87817,
	0x00007611,
	0x03400615,
	0x03404715,
	0x05434021,
	0x00003041,
	0x00003841,
	0x011180a1,
	0x00003041,
	0x00000201,
	0x00000201,
	0x00000201,
	0x00000201,
	0x00000201,
	0xffffff11,
	0xfffeb837,
	0xfffc8027,
};

const uint32_t mme9097_draw_arrays_indirect_count[] = {
	0x01438515,
	0x03400021,
/* 0x0009: daic_draw_again */
	0x00002e31,
	0x00000701,
	0x00000501,
/* 0x0017: daic_again */
	0x0005ad10,
	0x00086807,
/* 0x001f: daic_end */
	0x007f4312,
	0x0007d837,
/* 0x0024: daic_restore */
/* 0x0027: daic_runout */
	0x00000201,
	0x00000301,
/* 0x002c: daic_runout_check */
	0x00d74451,
	0x0004d807,
	0x00002431,
	0x0638c021,
	0x00680041,
	0x00000041,
	0x00002041,
	0x00003041,
	0x01438021,
	0x00002041,
	0x00004411,
	0xd0400912,
	0x01618021,
	0x00000841,
	0x00d78021,
	0x00001041,
	0x00051b10,
	0x01614071,
	0xfffe9817,
	0xd0410912,
	0xffffed11,
	0x00032807,
	0xffffff11,
	0xfff9f817,
	0x00007611,
	0x03400515,
	0x014380a1,
	0x00002841,
	0x00000201,
	0x00000201,
	0x00000201,
	0x00000201,
	0xffffff11,
	0xfffef837,
	0xfffdc027,
};

const uint32_t mme9097_query_buffer_write[] = {
	0x00000201,
	0x00000301,
/* 0x000b: qbw_ready */
	0x00000401,
	0x05b00551,
/* 0x0012: qbw_clamp */
/* 0x0013: qbw_postclamp */
	0x00000601,
	0x00000701,
	0x0005be10,
	0x00060610,
/* 0x0020: qbw_done */
	0x00000701,
	0x0000b087,
	0x00000601,
	0x00051210,
	0x0001c807,
	0x00075b10,
	0x00011837,
	0x00048c10,
	0x00060410,
	0x0000a027,
	0x00000a11,
	0x00003841,
	0x00003041,
	0x00001041,
	0x00028817,
	0x04000411,
	0x84010042,
	0x05b00021,
	0x00010511,
	0x00017610,
	0x00023f10,
	0x00003841,
	0x00003041,
	0x00001841,
	0x840100c2,
	0x00110071,
};

const uint32_t mme9097_conservative_raster_state[] = {
	0x07400021,
	0x00000041,
	0xb8d04042,
/* 0x000c: crs_loop */
	0x0001c211,
	0xb8c08042,
	0x06310021,
	0x020c4211,
	0x5b008042,
	0x01004212,
	0x41085212,
	0x20a1c021,
	0x00040311,
	0xffffdb11,
	0xffffd817,
	0x00001041,
	0x051480a1,
	0x00004041,
};

const uint32_t mme9097_set_priv_reg[] = {
	0x04110021,
	0x00000041,
	0x00000201,
/* 0x000f: spr_wait_loop */
	0x00000301,
	0x07400021,
/* 0x0014: spr_unk_else */
/* 0x0015: spr_unk_else_loop */
	0x00000041,
/* 0x0018: spr_exit */
	0x00001041,
	0x00001841,
	0x03468215,
	0x06310021,
	0x00000841,
	0x02008112,
	0xffff8911,
	0x00100021,
	0x00018837,
	0x03400115,
	0x00000041,
	0xffffc911,
	0xffff4837,
	0x00014027,
	0x00028111,
	0x00000041,
	0xffffc911,
	0xffff88b7,
	0x00000991,
	0x00000911,
};

const uint32_t mme9097_compute_counter[] = {
/* 0x0003: iic_loop_start */
	0x00000f11,
/* 0x0006: iic_mul_start */
/* 0x0007: iic_mul_body */
	0x00004111,
	0x00000211,
/* 0x000c: iic_mul_cont */
/* 0x000f: iic_mul_done */
	0x00000301,
	0x00000411,
	0x00000511,
	0x00025827,
	0x0040c612,
	0x00013007,
	0x07c2c312,
	0x00006410,
	0x0002ad10,
	0x00004910,
	0xfffe4007,
	0x00029210,
	0xffffff11,
	0x00002111,
	0xfffcb817,
	0x00002a11,
	0x03410315,
	0x03414415,
	0x07410021,
	0x000058c0,
	0x0002a040,
};

const uint32_t mme9097_compute_counter_to_query[] = {
	0x00000201,
	0x03410315,
	0x03414415,
	0x0000c910,
	0x00031210,
	0x05b00351,
	0x00001c31,
	0x00002041,
	0x00000841,
	0x04000511,
	0x84014042,
	0x00010111,
	0x00006410,
	0x00021b10,
	0x05b00021,
	0x00001841,
	0x00002041,
	0x000010c1,
	0x84014042,
};