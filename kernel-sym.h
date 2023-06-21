void UpdateVoltages(
	N_3DArray<GiNaC::ex>& volt,
	N_3DArray<GiNaC::ex>& curr,
	N_3DArray<GiNaC::symbol>& vv,
	N_3DArray<GiNaC::symbol>& vi,
	unsigned int startX, unsigned int endX,
	unsigned int startY, unsigned int endY,
	unsigned int startZ, unsigned int endZ
);

void UpdateCurrents(
	N_3DArray<GiNaC::ex>& curr,
	N_3DArray<GiNaC::ex>& volt,
	N_3DArray<GiNaC::symbol>& iv,
	N_3DArray<GiNaC::symbol>& ii,
	unsigned int startX, unsigned int endX,
	unsigned int startY, unsigned int endY,
	unsigned int startZ, unsigned int endZ
);
