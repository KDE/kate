/* This file is part of the KDE project
   Copyright (C) 2014 Martin Sandsmark <martin.sandsmark@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef REPLICODESETTINGS_H
#define REPLICODESETTINGS_H

#include <QObject>

class QIODevice;

class ReplicodeSettings : public QObject
{
    Q_OBJECT
public:
    explicit ReplicodeSettings(QObject *parent = nullptr);
    void load();
    void save();
    void setDefaults();

    ///////
    // Load
    QString userOperatorPath;
    QString userClassPath;
    QString sourcePath;

    ///////
    // Init
    int basePeriod;
    int reductionCoreCount;
    int timeCoreCount;

    /////////
    // System
    int perfSamplingPeriod;
    float floatTolerance;
    int timeTolerance;
    int primaryTimeHorizon;
    int secondaryTimeHorizon;

    // Model
    float mdlInertiaSuccessRateThreshold;
    int mdlInertiaCountThreshold;

    // Targeted Pattern Extractor
    float tpxDeltaSuccessRateThreshold;
    int tpxTimehorizon;

    // Simulation
    int minimumSimulationTimeHorizon;
    int maximumSimulationTimeHorizon;
    float simulationTimeHorizon;

    ////////
    // Debug
    bool debug;
    int notificationMarkerResilience;
    int goalPredictionSuccessResilience;
    int debugWindows;
    int traceLevels;

    bool getObjects;
    bool decompileObjects;
    QString decompilationFilePath;
    bool ignoreNamedObjects;
    QString objectsPath;
    bool testObjects;

    //////
    // Run
    int runTime;
    int probeLevel;

    bool getModels;
    bool decompileModels;
    bool ignoreNamedModels;
    QString modelsPath;
    bool testModels;

};

#endif // REPLICODESETTINGS_H
