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

#include "replicodesettings.h"
#include <QXmlStreamWriter>
#include <QSettings>
#include <QDebug>
#include <QString>

ReplicodeSettings::ReplicodeSettings(QObject *parent) :
    QObject(parent)
{
    load();
}

void ReplicodeSettings::load()
{
    QSettings settings("MTS", "Humanobs UI");

    // Load
    userOperatorPath = settings.value("User Operator Module Path", QString()).toString();
    userClassPath = settings.value("User Class File Path", QString()).toString();

    // Int
    basePeriod = settings.value("Base Period", 50000).toInt();
    reductionCoreCount = settings.value("Reduction Core Count", 6).toInt();
    timeCoreCount = settings.value("Time Core Count", 2).toInt();

    // System
    mdlInertiaSuccessRateThreshold = settings.value("Model Inertia Success Rate Threshold", 0.9).toFloat();
    mdlInertiaCountThreshold = settings.value("Model Inertia Count Threshold", 6).toInt();
    tpxDeltaSuccessRateThreshold = settings.value("Targeted Pattern Extractor Delta Success Rate Threshold", 0.1).toFloat();
    minimumSimulationTimeHorizon = settings.value("Minimum Simulation Time Horizon", 0).toInt();
    maximumSimulationTimeHorizon = settings.value("Maximum Simulation Time Horizon", 0).toInt();
    simulationTimeHorizon = settings.value("Simulation Time Horizon", 0.3).toFloat();
    tpxTimehorizon = settings.value("Targeted Pattern Extractor Time Horizon", 500000).toInt();
    perfSamplingPeriod = settings.value("Perf Sampling Period", 250000).toInt();
    floatTolerance = settings.value("Float Tolerance", 0.00001).toFloat();
    timeTolerance = settings.value("Timer Tolerance", 10000).toInt();
    primaryTimeHorizon = settings.value("Primary Time Horizon", 3600000).toInt();
    secondaryTimeHorizon = settings.value("Secondary Time Horizon", 7200000).toInt();
    //Debug
    debug = settings.value("Debug", true).toBool();
    debugWindows = settings.value("Debug Windows", 1).toInt();
    traceLevels = settings.value("Trace Levels", "CC").toString().toInt(0, 16);
    // Debug Resilience
    notificationMarkerResilience = settings.value("Notification Marker Resilience", 1).toInt();
    goalPredictionSuccessResilience = settings.value("Goal Prediction Success Resilience", 1000).toInt();
    // Debug Objects
    getObjects = settings.value("Get Objects", true).toBool();
    decompileObjects = settings.value("Decompile Objects", true).toBool();
    decompilationFilePath = settings.value("Decompilation Files Paths", QString()).toString();
    ignoreNamedObjects = settings.value("Ignore Named Objects", false).toBool();
    objectsPath = settings.value("Objects Path", QString()).toString();
    testObjects = settings.value("Test Objects", false).toBool();
    // Run
    runTime = settings.value("Run Time", 1080).toInt();
    probeLevel = settings.value("Probe Level", 2).toInt();
    getModels = settings.value("Get Models", false).toBool();
    decompileModels = settings.value("Decompile Models", false).toBool();
    ignoreNamedModels = settings.value("Ignore Named Models", true).toBool();
    modelsPath = settings.value("Models Path", QString()).toString();
    testModels = settings.value("Test Models", false).toBool();
}

void ReplicodeSettings::save()
{
    QSettings settings("MTS", "Humanobs UI");

    // Load
    settings.setValue("User Operator Module Path", userOperatorPath);
    settings.setValue("User Class File Path", userClassPath);

    // Init
    settings.setValue("Base Period", basePeriod);
    settings.setValue("Reduction Core Count", reductionCoreCount);
    settings.setValue("Time Core Count", timeCoreCount);

    // System
    settings.setValue("Model Inertia Success Rate Threshold", mdlInertiaSuccessRateThreshold);
    settings.setValue("Model Inertia Count Threshold", mdlInertiaCountThreshold);
    settings.setValue("Targeted Pattern Extractor Delta Success Rate Threshold", tpxDeltaSuccessRateThreshold);
    settings.setValue("Minimum Simulation Time Horizon", minimumSimulationTimeHorizon);
    settings.setValue("Maximum Simulation Time Horizon", maximumSimulationTimeHorizon);
    settings.setValue("Simulation Time Horizon", simulationTimeHorizon);
    settings.setValue("Targeted Pattern Extractor Time Horizon", tpxTimehorizon);
    settings.setValue("Perf Sampling Period", perfSamplingPeriod);
    settings.setValue("Float Tolerance", floatTolerance);
    settings.setValue("Timer Tolerance", timeTolerance);
    settings.setValue("Primary Time Horizon", primaryTimeHorizon);
    settings.setValue("Secondary Time Horizon", secondaryTimeHorizon);
    //Debug
    settings.setValue("Debug", debug);
    settings.setValue("Debug Windows", debugWindows);
    settings.setValue("Trace Levels", QString::number(traceLevels, 16));
    // Debug Resilience
    settings.setValue("Notification Marker Resilience", notificationMarkerResilience);
    settings.setValue("Goal Prediction Success Resilience", goalPredictionSuccessResilience);
    // Debug Objects
    settings.setValue("Get Objects", getObjects);
    settings.setValue("Decompile Objects", decompileObjects);
    settings.setValue("Decompilation Files Paths", decompilationFilePath);
    settings.setValue("Ignore Named Objects", ignoreNamedObjects);
    settings.setValue("Objects Path", objectsPath);
    settings.setValue("Test Objects", testObjects);
    // Run
    settings.setValue("Run Time", runTime);
    settings.setValue("Probe Level", probeLevel);
    settings.setValue("Get Models", getModels);
    settings.setValue("Decompile Models", decompileModels);
    settings.setValue("Ignore Named Models", ignoreNamedModels);
    settings.setValue("Models Path", modelsPath);
    settings.setValue("Test Models", testModels);
}

void ReplicodeSettings::setDefaults()
{
    // Load
    userOperatorPath = QString();
    userClassPath = QString();

    // Init
    basePeriod = 50000;
    reductionCoreCount = 6;
    timeCoreCount = 2;

    // System
    mdlInertiaSuccessRateThreshold = 0.9;
    mdlInertiaCountThreshold = 6;
    tpxDeltaSuccessRateThreshold = 0.1;
    minimumSimulationTimeHorizon = 0;
    maximumSimulationTimeHorizon = 0;
    simulationTimeHorizon = 0.3;
    tpxTimehorizon = 500000;
    perfSamplingPeriod = 250000;
    floatTolerance = 0.00001;
    timeTolerance = 10000;
    primaryTimeHorizon = 3600000;
    secondaryTimeHorizon = 7200000;

    //Debug
    debug = true;
    debugWindows = 1;
    traceLevels = 0xCC;

    // Debug Resilience
    notificationMarkerResilience = 1;
    goalPredictionSuccessResilience = 1000;

    // Debug Objects
    getObjects = true;
    decompileObjects = true;
    decompilationFilePath = QString();
    ignoreNamedObjects = false;
    objectsPath = QString();
    testObjects = false;

    // Run
    runTime = 1080;
    probeLevel = 2;
    getModels = false;
    decompileModels = false;
    ignoreNamedModels = true;
    modelsPath = QString();
    testModels = false;
}



void ReplicodeSettings::writeXml(QIODevice* output, QString sourceFile) const
{
    QXmlStreamWriter writer(output);
    writer.setAutoFormatting(true);
    writer.writeStartElement("RunConfiguration");

    writer.writeEmptyElement("Load");
    writer.writeAttribute("usr_operator_path", userOperatorPath);
    writer.writeAttribute("usr_class_path", userClassPath);
    writer.writeAttribute("source_file_name", sourceFile);

    writer.writeEmptyElement("Init");
    writer.writeAttribute("base_period", QString::number(basePeriod));
    writer.writeAttribute("reduction_core_count", QString::number(reductionCoreCount));
    writer.writeAttribute("time_core_count", QString::number(timeCoreCount));

    writer.writeEmptyElement("System");
    writer.writeAttribute("mdl_inertia_sr_thr", QString::number(mdlInertiaSuccessRateThreshold));
    writer.writeAttribute("mdl_inertia_cnt_thr", QString::number(mdlInertiaCountThreshold));
    writer.writeAttribute("tpx_dsr_thr", QString::number(tpxDeltaSuccessRateThreshold));
    writer.writeAttribute("min_sim_time_horizon", QString::number(minimumSimulationTimeHorizon));
    writer.writeAttribute("max_sim_time_horizon", QString::number(maximumSimulationTimeHorizon));
    writer.writeAttribute("sim_time_horizon", QString::number(simulationTimeHorizon));
    writer.writeAttribute("tpx_time_horizon", QString::number(tpxTimehorizon));
    writer.writeAttribute("perf_sampling_period", QString::number(perfSamplingPeriod));
    writer.writeAttribute("float_tolerance", QString::number(floatTolerance));
    writer.writeAttribute("time_tolerance", QString::number(timeTolerance));
    writer.writeAttribute("primary_thz", QString::number(primaryTimeHorizon));
    writer.writeAttribute("secondary_thz", QString::number(secondaryTimeHorizon));

    writer.writeStartElement("Debug");
    writer.writeAttribute("debug", debug ? "yes" : "no");
    writer.writeAttribute("debug_windows", QString::number(debugWindows));
    writer.writeAttribute("trace_levels", QString::number(traceLevels, 16));
    writer.writeEmptyElement("Resilience");
    writer.writeAttribute("ntf_mk_resilience", QString::number(notificationMarkerResilience));
    writer.writeAttribute("goal_pred_success_resilience", QString::number(goalPredictionSuccessResilience));
    writer.writeEmptyElement("Objects");
    writer.writeAttribute("get_objects", getObjects ? "yes" : "no");
    writer.writeAttribute("decompile_objects", decompileObjects ? "yes" : "no");
    writer.writeAttribute("decompile_to_file", !decompilationFilePath.isEmpty() ? "yes" : "no");
    writer.writeAttribute("decompilation_file_path", decompilationFilePath);
    writer.writeAttribute("ignore_named_objects", ignoreNamedObjects ? "yes" : "no");
    writer.writeAttribute("write_objects", !objectsPath.isEmpty() ? "yes" : "no");
    writer.writeAttribute("objects_path", objectsPath);
    writer.writeAttribute("test_objects", testObjects ? "yes" : "no");
    writer.writeEndElement(); // Debug

    writer.writeStartElement("Run");
    writer.writeAttribute("run_time", QString::number(runTime));
    qDebug() << "run_time:" << QString::number(runTime);
    writer.writeAttribute("probe_level", QString::number(probeLevel));
    writer.writeEmptyElement("Models");
    writer.writeAttribute("get_models", getModels ? "yes" : "no");
    writer.writeAttribute("decompile_models", decompileModels ? "yes" : "no");
    writer.writeAttribute("ignore_named_models", ignoreNamedModels ? "yes" : "no");
    writer.writeAttribute("write_models", !modelsPath.isEmpty() ? "yes" : "no");
    writer.writeAttribute("models_path", modelsPath);
    writer.writeAttribute("test_models", testModels ? "yes" : "no");
    writer.writeEndElement(); // Run

    writer.writeEndElement(); // RunConfiguration
}
