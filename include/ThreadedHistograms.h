#ifndef JAEASortThreadedHistograms
#define JAEASortThreadedHistograms

#include <ThreadedHistogramList.h>

#include <TH1F.h>
#include <TH2F.h>
#include <TH3.h>
#include <TH3F.h>

#include <memory>

#define JAEA_THREADED_DETECTOR_HISTOGRAM_LIST(X) \
    X(TH1F, s3_raw_ring_mult, "s3_raw", "S3 raw ring multiplicity", 11, -0.5, 10.5) \
    X(TH1F, s3_raw_sector_mult, "s3_raw", "S3 raw sector multiplicity", 11, -0.5, 10.5) \
    X(TH2F, s3_raw_ring_energy, "s3_raw", "S3 raw ring energy;Ring;Ring energy", 24, -0.5, 23.5, 1024, 0, 8192) \
    X(TH2F, s3_raw_sector_energy, "s3_raw", "S3 raw sector energy;Sector;Sector energy", 32, -0.5, 31.5, 1024, 0, 8192) \
    X(TH2F, s3_raw_ring_sector, "s3_raw", "S3 raw ring vs sector;Sector;Ring", 32, -0.5, 31.5, 24, -0.5, 23.5) \
    X(TH2F, s3_raw_ring_sector_energy, "s3_raw", "S3 raw ring energy vs sector energy;Ring energy;Sector energy", 512, 0, 8192, 512, 0, 8192) \
    X(TH3I, s3_raw_sector_vs_sector_ring_energy, "s3_raw", "S3 raw sector vs sector energy vs ring energy;Sector;Sector energy;Ring energy", 32, -0.5, 31.5, 52, 0, 8192, 52, 0, 8192) \
    X(TH3I, s3_raw_ring_vs_sector_ring_energy, "s3_raw", "S3 raw ring vs sector energy vs ring energy;Ring;Sector energy;Ring energy", 24, -0.5, 23.5, 52, 0, 8192, 52, 0, 8192) \
    X(TH1F, s3_raw_ring_sector_dt, "s3_raw", "S3 raw ring-sector time difference;Ring - sector time", 201, -1005, 1005) \
    X(TH2F, s3_raw_ring_dt, "s3_raw", "S3 raw ring vs ring-sector time difference;Ring;Ring - sector time", 24, -0.5, 23.5, 201, -1005, 1005) \
    X(TH2F, s3_raw_sector_dt, "s3_raw", "S3 raw sector vs ring-sector time difference;Sector;Ring - sector time", 32, -0.5, 31.5, 201, -1005, 1005) \
    X(TH1F, s3_pixel_mult, "s3_pixels", "S3 built pixel multiplicity", 11, -0.5, 10.5) \
    X(TH2F, s3_pixel_ring_sector, "s3_pixels", "S3 built pixel ring vs sector;Sector;Ring", 32, -0.5, 31.5, 24, -0.5, 23.5) \
    X(TH1F, s3_pixel_energy, "s3_pixels", "S3 built pixel primary energy;Energy", 1024, 0, 8192) \
    X(TH2F, s3_pixel_ring_energy, "s3_pixels", "S3 built pixel ring vs primary energy;Ring;Energy", 24, -0.5, 23.5, 1024, 0, 8192) \
    X(TH2F, s3_pixel_sector_energy, "s3_pixels", "S3 built pixel sector vs primary energy;Sector;Energy", 32, -0.5, 31.5, 1024, 0, 8192) \
    X(TH2F, s3_pixel_ring_sector_energy, "s3_pixels", "S3 built pixel ring energy vs sector energy;Ring energy;Sector energy", 512, 0, 8192, 512, 0, 8192) \
    X(TH1F, s3_pixel_ring_sector_dt, "s3_pixels", "S3 built pixel ring-sector time difference;Ring - sector time", 201, -1005, 1005) \
    X(TH2F, s3_pixel_theta_energy, "s3_pixels", "S3 built pixel theta vs energy;Theta [rad];Energy", 180, 0, 3.14159265358979323846, 1024, 0, 8192) \
    X(TH2F, s3_pixel_position_xy, "s3_pixels", "S3 built pixel position;X;Y", 200, -50, 50, 200, -50, 50) \
    X(TH3F, s3_pixel_position_xyz, "s3_pixels", "S3 built pixel position;Z;X;Y", 120, -60, 60, 120, -60, 60, 120, -60, 60) \
    X(TH2F, cdte_chan, "gammas", "CdTe Evergy vs Channel;Channel;Energy", 16, -0.5, 15.5, 1000, 0, 500) \
    X(TH1F, cdte_energy, "gammas", "CdTe summed energy;Energy [keV]", 1000, 0, 500) \
    X(TH1F, cdte_energy_S3, "gammas", "CdTe summed energy (S3 gated);Energy [keV]", 1000, 0, 500) \
    X(TH2F, cdte_S3time, "gammas", "CdTe Channel vs S3-CdTe time;Channel;Time", 16, -0.5, 15.5, 201, -1000.5, 1000.5) \
    X(TH2F, cdte_S3time_gate, "gammas", "CdTe Channel vs S3-CdTe time;Channel;Time", 16, -0.5, 15.5, 201, -1000.5, 1000.5) \
    X(TH2F, cdte_S3, "gammas", "CdTe Evergy vs Channel (S3 Gated);Channel;Energy", 16, -0.5, 15.5, 1000, 0, 500) \
    X(TH1F, cdte_doppler, "gammas", "CdTe Doppler-corrected energy;Energy [keV]", 1000, 0, 500) \
    X(TH2F, cdte_ring_doppler, "gammas", "S3 ring vs CdTe Doppler-corrected energy;Ring;Energy [keV]", 24, -0.5, 23.5, 1000, 0, 500) \
    X(TH1F, cdte_doppler_beam, "kinematics_beam", "CdTe Doppler-corrected energy (beam);Energy [keV]", 1000, 0, 500) \
    X(TH2F, cdte_ring_doppler_beam, "kinematics_beam", "S3 ring vs CdTe Doppler-corrected energy (beam);Ring;Energy [keV]", 24, -0.5, 23.5, 1000, 0, 500) \
    X(TH3F, gamma_positions, "gammas", "Gamma detector hit positions;Z;X;Y", 400, -100, 100, 400, -100, 100, 400, -100, 100) \
    X(TH2F, hpge_chan, "gammas", "HPGe Evergy vs Channel;Channel;Energy", 16, -0.5, 15.5, 1000, 0, 1000) \
    X(TH1F, hpge_energy, "gammas", "HPGe summed energy;Energy [keV]", 4000, 0, 2000) \
    X(TH1F, hpge_energy_S3, "gammas", "HPGe summed energy (S3 gated);Energy [keV]", 4000, 0, 2000) \
    X(TH2F, hpge_S3time, "gammas", "HPGe Channel vs S3-HPGe time;Channel;Time", 16, -0.5, 15.5, 201, -1000.5, 1000.5) \
    X(TH2F, hpge_S3time_gate, "gammas", "HPGe Channel vs S3-HPGe time;Channel;Time", 16, -0.5, 15.5, 201, -1000.5, 1000.5) \
    X(TH2F, hpge_S3, "gammas", "HPGe Evergy vs Channel (S3 Gated);Channel;Energy", 16, -0.5, 15.5, 1000, 0, 1000) \
    X(TH1F, hpge_doppler, "gammas", "HPGe Doppler-corrected energy;Energy [keV]", 4000, 0, 2000) \
    X(TH2F, hpge_ring_doppler, "gammas", "S3 ring vs HPGe Doppler-corrected energy;Ring;Energy [keV]", 24, -0.5, 23.5, 1000, 0, 1000) \
    X(TH1F, hpge_doppler_beam, "kinematics_beam", "HPGe Doppler-corrected energy (beam);Energy [keV]", 4000, 0, 2000) \
    X(TH2F, hpge_ring_doppler_beam, "kinematics_beam", "S3 ring vs HPGe Doppler-corrected energy (beam);Ring;Energy [keV]", 24, -0.5, 23.5, 1000, 0, 1000) \
    X(TH2F, cdte_cdte, "gammagamma", "CdTe-CdTe energy;CdTe energy [keV];CdTe energy [keV]", 1000, 0, 500, 1000, 0, 500) \
    X(TH2F, hpge_hpge, "gammagamma", "HPGe-HPGe energy;HPGe energy [keV];HPGe energy [keV]", 1000, 0, 1000, 1000, 0, 1000) \
    X(TH2F, cdte_hpge, "gammagamma", "CdTe-HPGe energy;CdTe energy [keV];HPGe energy [keV]", 1000, 0, 500, 1000, 0, 1000) \
    X(TH2F, cdte_hpge_S3, "gammagamma", "CdTe-HPGe energy (S3 gated);CdTe energy [keV];HPGe energy [keV]", 1000, 0, 500, 1000, 0, 1000) \
    X(TH1F, cdte_cdte_dt, "gammagamma", "CdTe-CdTe time difference;#Deltat", 400, -2000, 2000) \
    X(TH1F, cdte_cdte_dt_gate, "gammagamma", "CdTe-CdTe time difference |#Deltat| < 100;#Deltat", 400, -2000, 2000) \
    X(TH1F, hpge_hpge_dt, "gammagamma", "HPGe-HPGe time difference;#Deltat", 400, -2000, 2000) \
    X(TH1F, hpge_hpge_dt_gate, "gammagamma", "HPGe-HPGe time difference |#Deltat| < 100;#Deltat", 400, -2000, 2000) \
    X(TH1F, cdte_hpge_dt, "gammagamma", "CdTe-HPGe time difference;#Deltat", 400, -2000, 2000) \
    X(TH1F, cdte_hpge_dt_gate, "gammagamma", "CdTe-HPGe time difference |#Deltat| < 100;#Deltat", 400, -2000, 2000)

constexpr int kS3RingKinematicsCount = 24;

struct HistogramRefs {
    #define JAEA_DECLARE_DETECTOR_REF(Type, Name, Directory, ...) Type* Name = nullptr;
        JAEA_THREADED_DETECTOR_HISTOGRAM_LIST(JAEA_DECLARE_DETECTOR_REF)
    #undef JAEA_DECLARE_DETECTOR_REF

    TH2F* CdTeKinematics[kS3RingKinematicsCount];
    TH2F* HPGeKinematics[kS3RingKinematicsCount];
    TH2F* CdTeKinematicsBeam[kS3RingKinematicsCount];
    TH2F* HPGeKinematicsBeam[kS3RingKinematicsCount];
};

class ThreadedHistogramSet : public ThreadedHistogramList {
public:
    #define JAEA_DECLARE_DETECTOR_THREADED_HIST(Type, Name, Directory, ...) std::unique_ptr<TThreadedObject<Type>> Name;
    JAEA_THREADED_DETECTOR_HISTOGRAM_LIST(JAEA_DECLARE_DETECTOR_THREADED_HIST)
    #undef JAEA_DECLARE_DETECTOR_THREADED_HIST

    std::unique_ptr<TThreadedObject<TH2F>> CdTeKinematics[kS3RingKinematicsCount];
    std::unique_ptr<TThreadedObject<TH2F>> HPGeKinematics[kS3RingKinematicsCount];
    std::unique_ptr<TThreadedObject<TH2F>> CdTeKinematicsBeam[kS3RingKinematicsCount];
    std::unique_ptr<TThreadedObject<TH2F>> HPGeKinematicsBeam[kS3RingKinematicsCount];

    ThreadedHistogramSet()
    {
        #define JAEA_REGISTER_DETECTOR_HIST(Type, Name, Directory, ...) \
            Name.reset(new TThreadedObject<Type>(#Name, __VA_ARGS__)); \
            Register(*Name, Directory);
        JAEA_THREADED_DETECTOR_HISTOGRAM_LIST(JAEA_REGISTER_DETECTOR_HIST)
        #undef JAEA_REGISTER_DETECTOR_HIST

        for (int i = 0; i < kS3RingKinematicsCount; ++i) {
            CdTeKinematics[i].reset(new TThreadedObject<TH2F>(
                Form("CdTe_kinematics_%d", i),
                Form("CdTe opening angle vs energy for S3 ring %d;Opening angle [deg];Energy [keV]", i),
                180, 0, 180, 1000, 0, 500));
            Register(*CdTeKinematics[i], "gammas/kinematics");
        }

        for (int i = 0; i < kS3RingKinematicsCount; ++i) {
            HPGeKinematics[i].reset(new TThreadedObject<TH2F>(
                Form("HPGe_kinematics_%d", i),
                Form("HPGe opening angle vs energy for S3 ring %d;Opening angle [deg];Energy [keV]", i),
                180, 0, 180, 1000, 0, 1000));
            Register(*HPGeKinematics[i], "gammas/kinematics");
        }

        for (int i = 0; i < kS3RingKinematicsCount; ++i) {
            CdTeKinematicsBeam[i].reset(new TThreadedObject<TH2F>(
                Form("CdTe_kinematics_beam_%d", i),
                Form("CdTe opening angle vs energy for S3 ring %d (beam);Opening angle [deg];Energy [keV]", i),
                180, 0, 180, 1000, 0, 500));
            Register(*CdTeKinematicsBeam[i], "kinematics_beam");
        }

        for (int i = 0; i < kS3RingKinematicsCount; ++i) {
            HPGeKinematicsBeam[i].reset(new TThreadedObject<TH2F>(
                Form("HPGe_kinematics_beam_%d", i),
                Form("HPGe opening angle vs energy for S3 ring %d (beam);Opening angle [deg];Energy [keV]", i),
                180, 0, 180, 1000, 0, 1000));
            Register(*HPGeKinematicsBeam[i], "kinematics_beam");
        }
    }

    HistogramRefs ResolveHistogramRefs()
    {
        HistogramRefs refs;

        #define JAEA_RESOLVE_DETECTOR_REF(Type, Name, Directory, ...) refs.Name = Name->Get().get();
        JAEA_THREADED_DETECTOR_HISTOGRAM_LIST(JAEA_RESOLVE_DETECTOR_REF)
        #undef JAEA_RESOLVE_DETECTOR_REF

        for (int i = 0; i < kS3RingKinematicsCount; ++i) {
            refs.CdTeKinematics[i] = CdTeKinematics[i]->Get().get();
        }
        for (int i = 0; i < kS3RingKinematicsCount; ++i) {
            refs.HPGeKinematics[i] = HPGeKinematics[i]->Get().get();
        }
        for (int i = 0; i < kS3RingKinematicsCount; ++i) {
            refs.CdTeKinematicsBeam[i] = CdTeKinematicsBeam[i]->Get().get();
        }
        for (int i = 0; i < kS3RingKinematicsCount; ++i) {
            refs.HPGeKinematicsBeam[i] = HPGeKinematicsBeam[i]->Get().get();
        }

        return refs;
    }
};

#endif
