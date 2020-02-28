#include "..\Header\G422.h"
#include "..\Header\G422_DVC.h"
#include "..\Header\G422_MDL_DVC.h"

void G422::cueEngines(OMS& eng, OMS::SIMSTATE sst)
{
	if (eng.state == sst) return;

	eng.state = sst;
}

void G422::simEngines(double& dT, OMS& eng)
{
	double fuelMass = GetPropellantMass(fuel_sys);

	if (eng.feed & OMS::FUEL_PUMP && (fuelMass < 0.001 || !(eng.feed & OMS::FUEL_OPEN)))
		clbkVCMouseEvent((VC_CTRLSET_SWITCHES << 16) | (VC_swIndexByMGID[MGID_SW2_SYSFEED_OMS] & 0xFFFF), PANEL_MOUSE_RBPRESSED, _V0);

	if (eng.state == OMS::SST_INOP) return;

	if (eng.feed & OMS::FUEL_PUMP)
	{
		if (eng.fuelPrs < 1)
		{
			double deltaPrs = 0.22 * dT;
			eng.fuelPrs = min(eng.fuelPrs + deltaPrs, 1);
		}
	}
	else
	{
		if (eng.fuelPrs > 0)
		{
			double deltaPrs = -0.12 * dT;
			eng.fuelPrs = max(eng.fuelPrs + deltaPrs, 0);
		}
	}

	if (fuelMass < 0.001) eng.fuelPrs = 0;

	if (eng.fuelPrs == 0) 
	{
		clbkVCMouseEvent((VC_CTRLSET_SWITCHES << 16) | (VC_swIndexByMGID[MGID_SW3_STBYIGN_OMS] & 0xFFFF), PANEL_MOUSE_RBPRESSED, _V0);
		return; 
	}

	if (thr_authority)
	{
		eng.thr = GetEngineLevel(ENGINE_MAIN);

		SetThrusterLevel_SingleStep(oms_thgr[0], eng.thr);
		SetThrusterLevel_SingleStep(oms_thgr[1], eng.thr);
		PlayVesselWave(SFXID, SFX_OMSBURN, NOLOOP, int(225 * eng.thr));
	}
}