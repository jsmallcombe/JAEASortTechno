double* ElasticRelativisticKinematics(
    double BeamLabAngleRadians,
    double IncidentLabEnergyOfBeam,
    double MassOfBeamInMeV,
    double MassOfTargetInMeV)
{
    static double result[5];

    const double theta = BeamLabAngleRadians;

    const double mBeam   = MassOfBeamInMeV;
    const double mTarget = MassOfTargetInMeV;

    //------------------------------------------
    // Incident beam
    //------------------------------------------

    const double EBeamTot =
        IncidentLabEnergyOfBeam + mBeam;

    const double pBeamIncident =
        std::sqrt(
            EBeamTot*EBeamTot
            - mBeam*mBeam);

    //------------------------------------------
    // Centre-of-mass velocity
    //------------------------------------------

    const double betaCM =
        pBeamIncident /
        (EBeamTot + mTarget);

    const double gammaCM =
        1.0/std::sqrt(
            1.0-betaCM*betaCM);

    const double bg =
        betaCM*gammaCM;

    const double bg2 =
        bg*bg;

    //------------------------------------------
    // Elastic case:
    // pcm1 = bg * AT * amu
    //------------------------------------------

    const double pcm =
        bg * mTarget;

    //------------------------------------------
    // Beam solution
    //------------------------------------------

    const double Ebg =
        std::sqrt(
            pcm*pcm + mBeam*mBeam)
        * bg;

    const double s =
        std::sin(theta);

    const double c =
        std::cos(theta);

    const double pA =
        Ebg*c;

    double pB =
        pcm*pcm
        - mBeam*mBeam*s*s*bg2;

    const double pC =
        1.0 + s*s*bg2;

    if(pB > 0)
        pB = gammaCM*std::sqrt(pB);
    else
        pB = 0;

    //------------------------------------------
    // High-energy branch
    //------------------------------------------

    const double pBeam =
        (pA+pB)/pC;

    //------------------------------------------
    // Beam CM angle
    //------------------------------------------

    const double thetaCM =
        std::acos(
            (pBeam*c-Ebg)
            /(pcm*gammaCM));

    //------------------------------------------
    // Recoil target
    //------------------------------------------

    const double thetaCMTarget =
        M_PI-thetaCM;

    const double y =
        std::sin(thetaCMTarget)*pcm;

    const double x =
        std::cos(thetaCMTarget)*pcm;

    const double xpTarget =
        gammaCM*x
        + bg*std::sqrt(
            pcm*pcm
            + mTarget*mTarget);

    const double pTarget =
        std::sqrt(
            xpTarget*xpTarget
            + y*y);

    //------------------------------------------
    // Betas
    //------------------------------------------

    const double betaBeam =
        pBeam /
        std::sqrt(
            pBeam*pBeam
            + mBeam*mBeam);

    const double betaTarget =
        pTarget /
        std::sqrt(
            pTarget*pTarget
            + mTarget*mTarget);

    //------------------------------------------
    // Original code used atan()
    //------------------------------------------

    const double thetaTargetLab =
        std::atan(y/xpTarget);

    result[0] = betaBeam;
    result[1] = betaTarget;
    result[2] = thetaTargetLab;
    result[3] = sqrt(pBeam*pBeam+mBeam*mBeam)-mBeam;
    result[4] = sqrt(pTarget*pTarget+mTarget*mTarget)-mTarget;

    return result;
}