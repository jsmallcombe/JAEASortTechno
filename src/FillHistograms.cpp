#include <FillHistograms.h>
#include <KinPhys.h>

#include <HistogramRuntime.h>
#include <IO.h>

#include <Math/VectorUtil.h>
#include <TMath.h>

#include <optional>

constexpr double defaultBeamMass = 53966.429;
constexpr double defaultTargetMass = 221742.905;
constexpr double defaultBeamE = 120;
constexpr double defaultTargetLoss = 5.7;
constexpr double defaultBeamLoss = 27.9;

double CalcBeta_Beam(double theta, double beamE, double beamMass, double targetMass){

    double tau = 1;
    double thetaCM;
    tau = beamMass / targetMass;
    if(std::sin(theta) > 1/tau){
        theta = std::asin(1/tau);
        if(theta < 0)
            theta += TMath::Pi();
    }
    thetaCM = std::asin(tau * std::sin(theta)) + theta;

    double term1 = std::pow(targetMass/(beamMass+targetMass),2);
    double term2 = 1 + tau*tau + 2*tau*std::cos(thetaCM);
    double term3 = beamE;

    double KE = term1 * term2 * term3;

    double gamma = KE / beamMass + 1;

    return std::sqrt(1 - 1/(gamma*gamma));

}
double CalcBeta_Target(double theta, double beamE, double beamMass, double targetMass){

    double tau = 1;
    double thetaCM;
    tau = beamMass / targetMass;
    if(std::sin(theta) > 1/tau){
        theta = std::asin(1/tau);
        if(theta < 0)
            theta += TMath::Pi();
    }
    thetaCM = std::asin(tau * std::sin(theta)) + theta;

    double term1 = beamMass * targetMass / std::pow((beamMass+targetMass),2);
    double term2 = 1 + tau*tau + 2*tau*std::cos(TMath::Pi() - thetaCM);
    double term3 = beamE;

    double KE = term1 * term2 * term3;

    double gamma = KE / targetMass + 1;

    return std::sqrt(1 - 1/(gamma*gamma));

}
double ThetaTarg_FromThetaBeam(double theta, double beamMass, double targetMass){

    double tau = 1;
    double thetaCM;
    tau = beamMass / targetMass;
    if(std::sin(theta) > 1/tau){
        theta = std::asin(1/tau);
        if(theta < 0)
            theta += TMath::Pi();
    }
    thetaCM = std::asin(tau * std::sin(theta)) + theta;

    double tanTheta = std::sin(TMath::Pi() - thetaCM)/(std::cos(TMath::Pi() - thetaCM) + 1);
    return std::atan(tanTheta);

}

DetHitScratch& DetHitScratchBuffer()
{
    thread_local DetHitScratch scratch;
    scratch.Clear();
    return scratch;
}

DetHitScratch& BuildDetHitCategories(const BuiltEventView& event)
{
    DetHitScratch& scratch = DetHitScratchBuffer();

    for (size_t i = 0; i < event.Size(); ++i) {
        const UShort_t mod = event.Mod[i];
        const UShort_t ch = event.Ch[i];

        switch (DetHit::GetDetType(mod, ch)) {
            case DetHit::HPGe:
                scratch.hpge.emplace_back(event.Ts[i], event.Adc[i], mod, ch);
                break;
            case DetHit::CdTe:
                scratch.cdte.emplace_back(event.Ts[i], event.Adc[i], mod, ch);
                break;
            case DetHit::S3Ring:
                if(event.Adc[i]<100)break;
                scratch.s3.AddRingHit(event.Ts[i], event.Adc[i], mod, ch);
                break;
            case DetHit::S3Sector:
                if(event.Adc[i]<100)break;
                scratch.s3.AddSectorHit(event.Ts[i], event.Adc[i], mod, ch);
                break;
            default:
                scratch.hits.emplace_back(event.Ts[i], event.Adc[i], mod, ch);
                break;
        }
    }
    
    return scratch;
}

const HistogramGateRefs& HistogramGateRefsBuffer()
{
    thread_local HistogramGateRefs gateRefs;
    thread_local bool initialized = false;

    if (!initialized) {
        if (gIO != nullptr) {

            
            bool kininp=(gIO->TestInput("BeamEnergy")||gIO->TestInput("BeamMass")||gIO->TestInput("TargetMass"));

            const double beamE = gIO->GetInput("BeamEnergy", defaultBeamE);
            const double beamMass = gIO->GetInput("BeamMass", defaultBeamMass);
            const double targetMass = gIO->GetInput("TargetMass", defaultTargetMass);

            const double targetLoss = gIO->GetInput("TargetLoss", defaultTargetLoss);
            const double beamLoss = gIO->GetInput("BeamLoss", defaultBeamLoss);

            gateRefs.invkin = gIO->GetGateConst(0);
            gateRefs.beta = gIO->GetGateConst(1);
            gateRefs.betabeam = gIO->GetGateConst(2);

            if(gIO->TestInput("GammaOffset")){
                gateRefs.gammaoffset = gIO->GetInput("GammaOffset", 1.0);
                gateRefs.goff = true;
            }

            gateRefs.invkinl = new TGraph();
            gateRefs.betal = new TGraph();
            gateRefs.betabeaml = new TGraph();

            for(int i=0;i<2000;i++){
                double theta=i*TMath::Pi()/2000.;
                // gateRefs.invkinl->SetPoint(gateRefs.invkinl->GetN(),theta,ThetaTarg_FromThetaBeam(theta, beamMass, targetMass));
                // gateRefs.betal->SetPoint(gateRefs.betal->GetN(),theta,CalcBeta_Target(theta, beamE, beamMass, targetMass));
                // gateRefs.betabeaml->SetPoint(gateRefs.betabeaml->GetN(),theta,CalcBeta_Beam(theta, beamE, beamMass, targetMass));

                double* kin=ElasticRelativisticKinematics(theta,beamE,beamMass,targetMass);
                gateRefs.invkinl->SetPoint(gateRefs.invkinl->GetN(),theta,kin[2]);
                // gateRefs.betal->SetPoint(gateRefs.betal->GetN(),theta,kin[1]);
                // gateRefs.betabeaml->SetPoint(gateRefs.betabeaml->GetN(),theta,kin[0]);

                // Adjust for angle, but using an average linear value for stopping power
                double BeamPostTargMeV =kin[3]-(beamLoss/abs(cos(theta)));
                double TargPostTargMeV =kin[4]-(targetLoss/abs(cos(kin[2])));
                if(BeamPostTargMeV<0)BeamPostTargMeV=0;
                if(TargPostTargMeV<0)TargPostTargMeV=0;
                double BetaTarg = sqrt(1 - 1/(pow(TargPostTargMeV/targetMass + 1,2)));
                double BetaBeam = sqrt(1 - 1/(pow(BeamPostTargMeV/beamMass + 1,2)));

                gateRefs.betal->SetPoint(gateRefs.betal->GetN(),theta,BetaTarg);
                gateRefs.betabeaml->SetPoint(gateRefs.betabeaml->GetN(),theta,BetaBeam);

                // cout<<endl<<theta<<" "<<kin[1]<<" "<<kin[4]<<" "<<TargPostTargMeV<<" "<<BetaTarg;
                // cout<<endl<<theta<<" "<<kin[0]<<" "<<kin[3]<<" "<<BeamPostTargMeV<<" "<<BetaBeam;
            }

            if(gateRefs.invkin&&!kininp){
                gateRefs.invkinSp=TSpline3("invkinSp",gateRefs.invkin);
            }else{
                gateRefs.invkinSp=TSpline3("invkinSp",gateRefs.invkinl);
            }

            if(gateRefs.beta&&!kininp){
                gateRefs.betaSp=TSpline3("betaSp",gateRefs.beta);
            }else{
                gateRefs.betaSp=TSpline3("betaSp",gateRefs.betal);
            }

            if(gateRefs.betabeam&&!kininp){
                gateRefs.betabeamSp=TSpline3("betabeamSp",gateRefs.betabeam);
            }else{
                gateRefs.betabeamSp=TSpline3("betabeamSp",gateRefs.betabeaml);
            }

            gateRefs.cdteS3up = gIO->GetInput("CdTeS3Up", 100);
            gateRefs.cdteS3down = gIO->GetInput("CdTeS3Down", -100);
            gateRefs.cdteS3backup = gIO->GetInput("CdTeS3BackUp", gateRefs.cdteS3up+1500);
            gateRefs.cdteS3backdown = gIO->GetInput("CdTeS3BackDown", gateRefs.cdteS3down)+1500;

            gateRefs.cdteS3tzero = gIO->GetInput("CdTeS3TZero", 0);
            gateRefs.cdtekevns = gIO->GetInput("CdTeKeVns", -1);
            gateRefs.cdteezero = gIO->GetInput("CdTeEZero", 120);

            gateRefs.hpgeS3up = gIO->GetInput("HpGeS3Up", 100);
            gateRefs.hpgeS3down = gIO->GetInput("HpGeS3Down", -100);
            gateRefs.hpgeS3backup = gIO->GetInput("HPGeS3BackUp", gateRefs.hpgeS3up+1500);
            gateRefs.hpgeS3backdown = gIO->GetInput("HPGeS3BackDown", gateRefs.hpgeS3down)+1500;
            gateRefs.pixelcut = gIO->GetInput("PixelCutE", 5);
        }
        initialized = true;
    }
    return gateRefs;
}

void FillHistograms(HistogramRefs& H, const BuiltEventView& event)
{
    const bool enableTimers = gHistogramRuntimeOptions.enableHistogramTimers;
    std::optional<HistogramChunkTimer> chunkTimer;
    if (enableTimers) chunkTimer.emplace(gHistogramRuntimeTimers.fillHistChunk1Ns);

    // Retrieve gate values from IO, initialized on the first thread call
    const HistogramGateRefs& gates = HistogramGateRefsBuffer();

    // Move these Next(...) lines to retime the section boundaries.
    if (enableTimers) chunkTimer->Next(gHistogramRuntimeTimers.fillHistChunk2Ns);

    // Build detector objects from basic events data`
    DetHitScratch& detHits = BuildDetHitCategories(event);
    auto& hpge = detHits.hpge;
    auto& cdte = detHits.cdte;
    auto& s3 = detHits.s3;
 
    if (enableTimers) chunkTimer->Next(gHistogramRuntimeTimers.fillHistChunk3Ns);

    for(size_t i = 0; i < cdte.size(); ++i) {
        auto& hit = cdte[i];
        const double hitEnergy = hit.Energy();
        const double hitTime = hit.Time();

        // H.cdte_ch_adc->Fill(hit.Ch(), hit.Adc());
        H.cdte_chan->Fill(hit.Index(), hitEnergy);
        H.cdte_energy->Fill(hitEnergy);

        for(size_t j = i + 1; j < cdte.size(); ++j) {
            auto& hitb = cdte[j];
            const double hitbTime = hitb.Time();

            const double dT = hitTime - hitbTime;
            H.cdte_cdte_dt->Fill(dT);
            if(std::abs(dT) < 100.0) {
                const double hitbEnergy = hitb.Energy();
                H.cdte_cdte_dt_gate->Fill(dT);
                H.cdte_cdte->Fill(hitEnergy, hitbEnergy);
                H.cdte_cdte->Fill(hitbEnergy, hitEnergy);
            }//cdte.cdte.dt
            if(hit.IsNeighbour(hitb)) {
                const double hitbEnergy = hitb.Energy();
                const double addbackEnergy = hitEnergy + hitbEnergy;
                H.cdte_addback_raw->Fill(addbackEnergy, hitEnergy);
                H.cdte_addback_raw->Fill(addbackEnergy, hitbEnergy);
                H.cdte_addback_time->Fill(dT, addbackEnergy);
            }//cdte.addback
        }//cdte.cdte

        for(auto&& hpgeHit : hpge) {
            const double dT = hitTime - hpgeHit.Time();
            H.cdte_hpge_dt->Fill(dT);
            if(std::abs(dT) < 100.0) {
                H.cdte_hpge_dt_gate->Fill(dT);
                H.cdte_hpge->Fill(hitEnergy, hpgeHit.Energy());
            }//cdte.hpge.dt
        }//cdte.hpge
    }//cdte


    for(size_t i = 0; i < hpge.size(); ++i) {
        auto& hit = hpge[i];
        const double hitEnergy = hit.Energy();
        const double hitTime = hit.Time();

        H.hpge_chan->Fill(hit.Index(), hitEnergy);
        H.hpge_energy->Fill(hitEnergy);

        for(size_t j =0; j < hpge.size(); ++j) {
            if(i==j)continue;
            auto& hitb = hpge[j];

            const double dT = hitTime - hitb.Time();
            H.hpge_hpge_dt->Fill(dT);

            if(hit.Index()==0)H.gamgamTHPGe1->Fill(dT,hitEnergy);
            if(hit.Index()==1)H.gamgamTHPGe2->Fill(dT,hitEnergy);
            if(hit.Index()==2)H.gamgamTHPGe3->Fill(dT,hitEnergy);
            if(hit.Index()==3)H.gamgamTHPGe4->Fill(dT,hitEnergy);
            if(hit.Index()==4)H.gamgamTHPGe5->Fill(dT,hitEnergy);
            if(hit.Index()==5)H.gamgamTHPGe6->Fill(dT,hitEnergy);


            if(std::abs(dT) < 100.0) {
                const double hitbEnergy = hitb.Energy();
                H.hpge_hpge_dt_gate->Fill(dT);
                H.hpge_hpge->Fill(hitEnergy, hitbEnergy);
                H.hpge_hpge->Fill(hitbEnergy, hitEnergy);
            }//hpge.hpge.dt
        }//hpge.hpge
    }//hpge


    H.s3_raw_ring_mult->Fill(s3.GetRingMultiplicity());
    H.s3_raw_sector_mult->Fill(s3.GetSectorMultiplicity());
    H.s3_pixel_mult->Fill(s3.GetPixelMultiplicity());

    for(auto& sector : s3.SectorHits()) {
        H.s3_raw_sector_energy->Fill(sector.Index(), sector.Energy());
    }//s3sector
    
    for(auto& ring : s3.RingHits()) {
        H.s3_raw_ring_energy->Fill(ring.Index(), ring.Energy());

        for(auto& sector : s3.SectorHits()) {
            const double dT = ring.Time() - sector.Time();
            H.s3_raw_ring_sector->Fill(sector.Index(), ring.Index());
            H.s3_raw_ring_sector_energy->Fill(ring.Energy(), sector.Energy());
            H.s3_raw_sector_ring_vs_sector_energy->Fill(sector.Index(),ring.Index(),sector.Energy());
            H.s3_raw_ring_sector_dt->Fill(dT);
            H.s3_raw_ring_dt->Fill(ring.Index(), dT);
            H.s3_raw_sector_dt->Fill(sector.Index(), dT);

            if(ring.Index() < kS3RingKinematicsCount && std::abs(dT)<200){
                H.Ringi_Sector[ring.Index()]->Fill(ring.Energy(), sector.Energy());
                H.Ringi_Sector_ch[ring.Index()]->Fill(ring.Charge(), sector.Energy());
            }
            if(sector.Index() < kS3SectorCount && std::abs(dT)<200){
                H.Sectori_Ring[sector.Index()]->Fill(sector.Energy(), ring.Energy());
                H.Sectori_Ring_ch[sector.Index()]->Fill(sector.Charge(), ring.Energy());
            }

        }//s3ring.s3sector
    }//s3ring


    if (enableTimers) chunkTimer->Next(gHistogramRuntimeTimers.fillHistChunk4Ns);

    std::vector<const HPGeHit*> gatedHpgeHits;
    gatedHpgeHits.reserve(hpge.size());
    std::vector<const CdTeHit*> gatedCdTeHits;
    gatedCdTeHits.reserve(cdte.size());

    for(auto& s3hit : s3.Hits()) {
        const UShort_t s3Sector = s3hit.Sector();
        const UShort_t s3Ring = s3hit.Ring();
        const double s3Energy = s3hit.Energy();
        const double s3Time = s3hit.Time();

        H.s3_pixel_ring_sector->Fill(s3Sector, s3Ring);
        H.s3_pixel_energy->Fill(s3Energy);
        H.s3_pixel_ring_energy->Fill(s3Ring, s3Energy);
        H.s3_pixel_sector_energy->Fill(s3Sector, s3Energy);
        const XYZVector pos = s3hit.Pos(true);
        H.s3_pixel_theta_energy->Fill(pos.Theta(), s3Energy);

        const DetHit* ring = s3hit.RingHit();
        const DetHit* sector = s3hit.SectorHit();
        if(ring != nullptr && sector != nullptr) {
            H.s3_pixel_ring_sector_dt->Fill(ring->Time() - sector->Time());
            H.s3_pixel_ring_sector_energy->Fill(ring->Energy(), sector->Energy());
        }

        H.s3_pixel_position_xy->Fill(pos.X(), pos.Y());

        if(s3Energy < gates.pixelcut) // Particle energy cut
            continue;

        if(ring != nullptr && sector != nullptr) {
            H.s3_pixel_ring_sector_dt_cut->Fill(ring->Time() - sector->Time());
            H.s3_pixel_ring_sector_energy_cut->Fill(ring->Energy(), sector->Energy());
        }

        H.s3_pixel_ring_sector_cut->Fill(s3Sector, s3Ring);
        H.s3_pixel_position_xy_cut->Fill(pos.X(), pos.Y());
        H.s3_pixel_theta_energy_cut->Fill(pos.Theta(), s3Energy);
        H.s3_pixel_position_xy_cut->Fill(pos.X(), pos.Y());

        H.s3_pixel_position_xyz->Fill(pos.Z(), pos.X(), pos.Y());


        // const double betaBeam = CalcBeta_Beam(pos.Theta());
        // const double beta = CalcBeta_Target(pos.Theta());

        const double beta = gates.betaSp.Eval(pos.Theta());
        const double betaBeam = gates.betabeamSp.Eval(pos.Theta());

        const double TargTheta = gates.invkinSp.Eval(pos.Theta());
        const double TargPhi = pos.Phi() + TMath::Pi();

        const XYZVector TargVec{ROOT::Math::Polar3DVector(gates.gammaoffset, TargTheta, TargPhi)};
            
        gatedHpgeHits.clear();
        for(auto& hit : hpge) {
            const double dT = hit.Time() - s3Time;
            H.hpge_S3time->Fill(dT);
            H.hpge_S3time_w->Fill(dT);
            H.hpge_chan_S3time->Fill(hit.Index(), dT);
            H.hpge_chan_S3time_w->Fill(hit.Index(), dT);
            XYZVector gammaPos = hit.Pos(true);
            const double openingAngleBeam = ROOT::Math::VectorUtil::Angle(pos, gammaPos);
            const double dopplerEnergyBeam = hit.DopplerCorrectedEnergy(openingAngleBeam, betaBeam);
            const XYZVector originalGammaPos = gammaPos;
            if(gates.goff)gammaPos -= TargVec;
            const double openingAngle = ROOT::Math::VectorUtil::Angle(TargVec, gammaPos);
            const double dopplerEnergy = hit.DopplerCorrectedEnergy(openingAngle, beta);
            H.HPGeES3dT->Fill(dT,hit.Energy());
            H.HPGeES3dTDopp->Fill(dT,dopplerEnergy);
            if(hit.Index()<6) H.HPGeS3TimeEnergy[hit.Index()]->Fill(dT,hit.Energy());


            if(dT > gates.hpgeS3down && dT < gates.hpgeS3up) {
                gatedHpgeHits.push_back(&hit);
                H.hpge_S3time_g->Fill(dT);
                
                // Moved here from hpge raw as position maths is a bit heavy, so limit the calls
                H.gamma_positions->Fill(originalGammaPos.Z(), originalGammaPos.X(), originalGammaPos.Y());

                H.hpge_chan_S3time_gate->Fill(hit.Index(), dT);
                H.hpge_chan_S3->Fill(hit.Index(), hit.Energy());
                H.hpge_chan_doppler->Fill(hit.Index(), dopplerEnergy);
                H.hpge_energy_S3->Fill(hit.Energy());
                H.hpge_doppler->Fill(dopplerEnergy);
                H.hpge_ring_doppler->Fill(s3Ring, dopplerEnergy);
                H.hpge_doppler_Beam->Fill(dopplerEnergyBeam);
                H.hpge_doppler_beam->Fill(dopplerEnergyBeam);
                H.hpge_chan_doppler_beam->Fill(hit.Index(), dopplerEnergyBeam);
                H.hpge_ring_doppler_beam->Fill(s3Ring, dopplerEnergyBeam);
                H.hpge_ring_doppler_Beam->Fill(s3Ring, dopplerEnergyBeam);
                H.HPGeKin->Fill(openingAngle * TMath::RadToDeg(), hit.Energy());
                H.HPGeKinBeam->Fill(openingAngleBeam * TMath::RadToDeg(), hit.Energy());
                H.HPGeKinDopp->Fill(openingAngle * TMath::RadToDeg(), dopplerEnergy);
                H.HPGeKinBeamDopp->Fill(openingAngleBeam * TMath::RadToDeg(), dopplerEnergyBeam);
                if(s3Ring < kS3RingKinematicsCount) {
                    H.HPGeKinematics[s3Ring]->Fill(openingAngle * TMath::RadToDeg(), hit.Energy());
                    H.HPGeKinematicsBeam[s3Ring]->Fill(openingAngleBeam * TMath::RadToDeg(), hit.Energy());
                }//s3.hpge.dt.hpgepixel
                for(auto&& hpgeHit : hpge) {
                    if(hit.Energy() != hpgeHit.Energy()){
                        const double dTgg = hit.Time() - hpgeHit.Time();
                        XYZVector gammaPos2 = hpgeHit.Pos(true);
                        if(gates.goff)gammaPos2 -= TargVec;
                        const double openingAngle_hpge = ROOT::Math::VectorUtil::Angle(TargVec, gammaPos2);
                        const double dopplerEnergyhpge = hpgeHit.DopplerCorrectedEnergy(openingAngle_hpge, beta);
                        if(std::abs(dTgg) < 100.0) {
                            H.hpge_hpge_dopp->Fill(dopplerEnergy, dopplerEnergyhpge);
                        }//cdte.hpge.dt
                    }
                }//cdte.hpge
            }//s3.hpge.dt
            if(dT > (gates.hpgeS3backdown) && dT < (gates.hpgeS3backup)){
                H.hpge_energy_S3_bg->Fill(hit.Energy());
                H.hpge_doppler_bg->Fill(dopplerEnergy);
                H.hpge_doppler_Beam_bg->Fill(dopplerEnergyBeam);
                H.hpge_ring_doppler_bg->Fill(s3Ring, dopplerEnergy);
                H.hpge_ring_doppler_Beam_bg->Fill(s3Ring, dopplerEnergyBeam);
                H.HPGeKinBeamDopp_bg->Fill(openingAngleBeam * TMath::RadToDeg(), dopplerEnergyBeam);
            }
        }//s3.hpge


        gatedCdTeHits.clear();
        for(auto& cdteHit : cdte) {
            const double dT = cdteHit.Time() - s3Time;
            const double cdteEnergy = cdteHit.Energy();

            H.cdte_S3time->Fill(dT);
            H.cdte_S3time_w->Fill(dT);
            H.cdte_chan_S3time->Fill(cdteHit.Index(), dT);
            H.cdte_chan_S3time_w->Fill(cdteHit.Index(), dT);
            if(cdteHit.Index()<16) H.CdTeS3TimeEnergy[cdteHit.Index()]->Fill(dT,cdteEnergy);
            XYZVector gammaPos = cdteHit.Pos(true);
            const double openingAngleBeam = ROOT::Math::VectorUtil::Angle(pos, gammaPos);
            const double dopplerEnergyBeam = cdteHit.DopplerCorrectedEnergy(openingAngleBeam, betaBeam);
            const XYZVector originalGammaPos = gammaPos;
            if(gates.goff)gammaPos -= TargVec;
            const double openingAngle = ROOT::Math::VectorUtil::Angle(TargVec, gammaPos);
            const double dopplerEnergy = cdteHit.DopplerCorrectedEnergy(openingAngle, beta);
            H.CdTeES3dT->Fill(dT,cdteEnergy);
            H.CdTeES3dTDopp->Fill(dT,dopplerEnergy);

            if(dT > gates.cdteS3down && dT < gates.cdteS3up) {

                const double CorrT = dT-gates.cdteS3tzero;

                double realignedE=cdteEnergy+((CorrT*gates.cdtekevns)*(cdteEnergy/gates.cdteezero));
                double dopplerrealignedE=DopplerCorrectEnergy(realignedE,openingAngle, beta);
                if(abs(CorrT)<100)H.cdte_corrected->Fill(realignedE);
                if(abs(CorrT)<100)H.cdte_corrected_doppler->Fill(dopplerrealignedE);
                H.CdTeES3dTcorr->Fill(dT,realignedE);
                H.CdTeES3dTDoppcorr->Fill(dT,dopplerrealignedE);

                H.gamma_positions->Fill(originalGammaPos.Z(), originalGammaPos.X(), originalGammaPos.Y());
                H.cdte_S3time_g->Fill(dT);

                H.cdte_chan_S3time_gate->Fill(cdteHit.Index(), dT);
                H.cdte_chan_S3->Fill(cdteHit.Index(), cdteEnergy);
                H.cdte_energy_S3->Fill(cdteEnergy);
                H.cdte_doppler->Fill(dopplerEnergy);
                H.cdte_chan_doppler->Fill(cdteHit.Index(),dopplerEnergy);
                H.cdte_ring_doppler->Fill(s3Ring, dopplerEnergy);
    
                H.cdte_doppler_beam->Fill(dopplerEnergyBeam);
                H.cdte_ring_doppler_beam->Fill(s3Ring, dopplerEnergyBeam);
                H.CdTeKin->Fill(openingAngle * TMath::RadToDeg(), cdteEnergy);
                H.CdTeKinBeam->Fill(openingAngleBeam * TMath::RadToDeg(), cdteEnergy);
                H.CdTeKinDopp->Fill(openingAngle * TMath::RadToDeg(), dopplerEnergy);
                H.CdTeKinBeamDopp->Fill(openingAngleBeam * TMath::RadToDeg(), dopplerEnergyBeam);
                if(s3Ring < kS3RingKinematicsCount) {
                    H.CdTeKinematics[s3Ring]->Fill(openingAngle * TMath::RadToDeg(), cdteEnergy);
                    H.CdTeKinematicsBeam[s3Ring]->Fill(openingAngleBeam * TMath::RadToDeg(), cdteEnergy);
                }//s3.cdte.dt.cdpixel

                for(const CdTeHit* gatedCdTeHit : gatedCdTeHits) {
                    const double gatedCdTeEnergy = gatedCdTeHit->Energy();
                    H.cdte_cdte_S3->Fill(cdteEnergy, gatedCdTeEnergy);
                    H.cdte_cdte_S3->Fill(gatedCdTeEnergy, cdteEnergy);
                    if(cdteHit.IsNeighbour(*gatedCdTeHit)) {
                        const double dTcc = cdteHit.Time() - gatedCdTeHit->Time();
                        const double addbackEnergy = cdteEnergy + gatedCdTeEnergy;
                        XYZVector addbackPos = cdteEnergy >= gatedCdTeEnergy ? originalGammaPos : gatedCdTeHit->Pos(true);
                        if(gates.goff)addbackPos -= TargVec;
                        const double addbackAngle = ROOT::Math::VectorUtil::Angle(TargVec, addbackPos);
                        const double dopplerAddbackEnergy = DopplerCorrectEnergy(addbackEnergy, addbackAngle, beta);
                        H.cdte_addback_doppler_raw->Fill(dopplerAddbackEnergy, cdteEnergy);
                        H.cdte_addback_doppler_raw->Fill(dopplerAddbackEnergy, gatedCdTeEnergy);
                        H.cdte_addback_doppler_time->Fill(dTcc, dopplerAddbackEnergy);
                    }//s3.cdte.addback
                }
                gatedCdTeHits.push_back(&cdteHit);

                for(const HPGeHit* hpgeHit : gatedHpgeHits) {
                    H.cdte_hpge_S3->Fill(cdteEnergy, hpgeHit->Energy());
                }//s3.cdte.dt.hpge

                for(auto&& hpgeHit : hpge) {
                    const double dTcg = cdteHit.Time() - hpgeHit.Time();
                    XYZVector gammaPos2 = hpgeHit.Pos(true);
                    if(gates.goff)gammaPos2 -= TargVec;
                    const double openingAngle_hpge = ROOT::Math::VectorUtil::Angle(TargVec, gammaPos2);
                    const double dopplerEnergyhpge = hpgeHit.DopplerCorrectedEnergy(openingAngle_hpge, beta);
                    if(std::abs(dTcg) < 100.0) {
                        H.cdte_hpge_dopp->Fill(dopplerEnergy, dopplerEnergyhpge);
                    }//cdte.hpge.dt
                }//cdte.hpge
            }//s3.cdte.dt
            
            if(dT > (gates.cdteS3backdown) && dT < (gates.cdteS3backup )){

                H.cdte_energy_S3_bg->Fill(cdteEnergy);
                H.cdte_doppler_bg->Fill(dopplerEnergy);
                H.cdte_ring_doppler_back->Fill(s3Ring, dopplerEnergy);

            }
        }//s3.cdte

    }//s3hit
}
