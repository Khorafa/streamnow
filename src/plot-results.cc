#include "ns3/gnuplot.h"
#include <algorithm> // Necesario para std::sort
#include <cmath>     // Necesario para sqrt y pow
#include <cstdlib>
#include <fstream>
#include <iomanip> // Necesario para std::fixed y std::setprecision
#include <iostream>
#include <map>
#include <numeric> // Necesario para std::accumulate
#include <string>
#include <vector>

using namespace ns3;

// Estructura para almacenar las  muestras de cada métrica
struct MetricSamples
{
    std::vector<double> lossSamples;
    std::vector<double> delaySamples;
    std::vector<double> jitterSamples;
};

// Estructura para almacenar los resultados ya procesados (media y margen de error)
struct ProcessedPoint
{
    double bitrate;
    double lossMean;
    double lossMarginOfError;
    double delayMean;
    double delayMarginOfError;
    double jitterMean;
    double jitterMarginOfError;
};

// Función para ejecutar un comando del sistema
void
ExecuteCommand(const std::string& command)
{
    if (system(command.c_str()))
    {
        
    }
}

// Función para calcular la media
double
CalculateMean(const std::vector<double>& samples)
{
    if (samples.empty())
    {
        return 0.0;
    }
    double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
    return sum / samples.size();
}

// Funciones para calcular la desviación estándar y el margen de error del 95%
double
CalculateStdDev(const std::vector<double>& samples, double mean)
{
    if (samples.size() < 2)
    {
        return 0.0;
    }
    double sq_sum = 0.0;
    for (const auto& s : samples)
    {
        sq_sum += std::pow(s - mean, 2);
    }
    return std::sqrt(sq_sum / (samples.size() - 1));
}

double
CalculateMarginOfError(double stdDev, int sampleSize)
{
    if (sampleSize < 2)
        return 0.0;
    const double T_VALUE_95_10_SAMPLES = 2.262; // Valor crítico de t-Student para 95% y n=10
    return T_VALUE_95_10_SAMPLES * stdDev / std::sqrt(sampleSize);
}

int
main(int argc, char* argv[])
{
    // --- UMBRALES DE CALIDAD DE SERVICIO (QoS) ---
    const double MAX_DELAY_MS = 20.0;
    const double MAX_JITTER_MS = 8.0;
    const double MAX_LOSS_PERCENT = 0.5;

    // --- CREAR DIRECTORIOS ---
    const char* dir = "graficas";
    const char* precision_dir = "graficas-precision";
    ExecuteCommand("mkdir -p " + std::string(dir));
    ExecuteCommand("mkdir -p " + std::string(precision_dir));

    // --- LECTURA DEL FICHERO DE DATOS ---
    std::map<int, std::map<double, MetricSamples>> dataByUsersAndBitrate;
    std::ifstream inFile("results.dat");
    if (!inFile)
    {
        std::cerr << "Error: No se puede abrir el fichero results.dat." << std::endl;
        return 1;
    }

    int numUsers;
    double tempBitrate, tempLoss, tempDelay, tempJitter;
    while (inFile >> numUsers >> tempBitrate >> tempLoss >> tempDelay >> tempJitter)
    {
        dataByUsersAndBitrate[numUsers][tempBitrate].lossSamples.push_back(tempLoss);
        dataByUsersAndBitrate[numUsers][tempBitrate].delaySamples.push_back(tempDelay);
        dataByUsersAndBitrate[numUsers][tempBitrate].jitterSamples.push_back(tempJitter);
    }
    inFile.close();

    // --- CÁLCULO ESTADÍSTICO Y PREPARACIÓN DE DATOS ---
    std::map<int, std::vector<ProcessedPoint>> processedDataByUserCount;
    for (auto const& [users, bitrateData] : dataByUsersAndBitrate)
    {
        for (auto const& [bitrate, samples] : bitrateData)
        {
            ProcessedPoint p_point;
            p_point.bitrate = bitrate;

            p_point.delayMean = CalculateMean(samples.delaySamples);
            double delayStdDev = CalculateStdDev(samples.delaySamples, p_point.delayMean);
            p_point.delayMarginOfError =
                CalculateMarginOfError(delayStdDev, samples.delaySamples.size());

            p_point.jitterMean = CalculateMean(samples.jitterSamples);
            double jitterStdDev = CalculateStdDev(samples.jitterSamples, p_point.jitterMean);
            p_point.jitterMarginOfError =
                CalculateMarginOfError(jitterStdDev, samples.jitterSamples.size());

            p_point.lossMean = CalculateMean(samples.lossSamples);
            double lossStdDev = CalculateStdDev(samples.lossSamples, p_point.lossMean);
            p_point.lossMarginOfError =
                CalculateMarginOfError(lossStdDev, samples.lossSamples.size());

            processedDataByUserCount[users].push_back(p_point);
        }
    }

    if (processedDataByUserCount.empty())
    {
        std::cerr << "Error: No se pudieron procesar los datos del fichero." << std::endl;
        return 1;
    }

    // Ordenar los puntos por bitrate
    double minBitrate = 1e9, maxBitrate = 0;
    for (auto& pair : processedDataByUserCount)
    {
        std::sort(pair.second.begin(),
                  pair.second.end(),
                  [](const ProcessedPoint& a, const ProcessedPoint& b) {
                      return a.bitrate < b.bitrate;
                  });
        if (!pair.second.empty())
        {
            if (pair.second.front().bitrate < minBitrate)
                minBitrate = pair.second.front().bitrate;
            if (pair.second.back().bitrate > maxBitrate)
                maxBitrate = pair.second.back().bitrate;
        }
    }

    // --- LÓGICA PARA ENCONTRAR EL BITRATE MÍNIMO ---
    std::vector<std::pair<int, ProcessedPoint>> finalResults;
    for (auto const& [users, points] : processedDataByUserCount)
    {
        ProcessedPoint bestPoint = {-1, -1, -1, -1, -1, -1, -1};
        for (const auto& point : points)
        {
            if (point.delayMean <= MAX_DELAY_MS && point.jitterMean <= MAX_JITTER_MS &&
                point.lossMean <= MAX_LOSS_PERCENT)
            {
                bestPoint = point;
                break;
            }
        }
        finalResults.push_back({users, bestPoint});
    }

    // --- GENERACIÓN DE FICHERO CSV CON RESUMEN ESTADÍSTICO ---
    std::ofstream summaryFile("sim_precision.dat");
    summaryFile << "Usuarios,Bitrate,LatenciaMedia,LatenciaError95,JitterMedio,JitterError95,"
                   "PerdidaMedia,PerdidaError95\n";
    for (auto const& [users, points] : processedDataByUserCount)
    {
        for (const auto& p : points)
        {
            summaryFile << users << "," << p.bitrate << "," << p.delayMean << ","
                        << p.delayMarginOfError << "," << p.jitterMean << ","
                        << p.jitterMarginOfError << "," << p.lossMean << ","
                        << p.lossMarginOfError << "\n";
        }
    }
    summaryFile.close();
    std::cout << "Fichero con resumen estadístico 'sim_precision.dat' generado correctamente."
              << std::endl;

    // --- GENERACIÓN DE GRÁFICAS DETALLADAS ---
    std::string file_prefix_detailed = std::string(dir) + "/grafica_ns3";

    // Gráfica de Retardo
    Gnuplot plotRetardo(file_prefix_detailed + "_retardo_detallado.png");
    plotRetardo.SetTitle("Retardo Medio vs. Bitrate");
    plotRetardo.SetTerminal("pngcairo enhanced font 'Helvetica,12' size 800,600");
    plotRetardo.SetLegend("Bitrate (Mbps)", "Retardo Medio (ms)");
    plotRetardo.SetExtra("set grid; set xtics 20;");
    for (auto const& [users, points] : processedDataByUserCount)
    {
        Gnuplot2dDataset dataset;
        dataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);
        dataset.SetTitle(std::to_string(users) + " Usuarios");
        for (const auto& point : points)
        {
            dataset.Add(point.bitrate, point.delayMean);
        }
        plotRetardo.AddDataset(dataset);
    }
    Gnuplot2dDataset delayThreshold;
    delayThreshold.SetTitle("Umbral (" + std::to_string((int) MAX_DELAY_MS) + " ms)");
    delayThreshold.SetStyle(Gnuplot2dDataset::LINES);
    delayThreshold.SetExtra("dt 2 lw 2");
    delayThreshold.Add(minBitrate, MAX_DELAY_MS);
    delayThreshold.Add(maxBitrate, MAX_DELAY_MS);
    plotRetardo.AddDataset(delayThreshold);
    std::ofstream plotFileRetardoOut(file_prefix_detailed + "_retardo_detallado.plt");
    plotRetardo.GenerateOutput(plotFileRetardoOut);
    plotFileRetardoOut.close();
    ExecuteCommand("gnuplot " + file_prefix_detailed + "_retardo_detallado.plt");
    ExecuteCommand("rm -f " + file_prefix_detailed + "_retardo_detallado.plt");

    // Gráfica de Jitter
    Gnuplot plotJitter(file_prefix_detailed + "_jitter_detallado.png");
    plotJitter.SetTitle("Jitter Medio vs. Bitrate");
    plotJitter.SetTerminal("pngcairo enhanced font 'Helvetica,12' size 800,600");
    plotJitter.SetLegend("Bitrate (Mbps)", "Jitter Medio (ms)");
    plotJitter.SetExtra("set grid; set xtics 20;");
    for (auto const& [users, points] : processedDataByUserCount)
    {
        Gnuplot2dDataset dataset;
        dataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);
        dataset.SetTitle(std::to_string(users) + " Usuarios");
        for (const auto& point : points)
        {
            dataset.Add(point.bitrate, point.jitterMean);
        }
        plotJitter.AddDataset(dataset);
    }
    Gnuplot2dDataset jitterThreshold;
    jitterThreshold.SetTitle("Umbral (" + std::to_string((int) MAX_JITTER_MS) + " ms)");
    jitterThreshold.SetStyle(Gnuplot2dDataset::LINES);
    jitterThreshold.SetExtra("dt 2 lw 2");
    jitterThreshold.Add(minBitrate, MAX_JITTER_MS);
    jitterThreshold.Add(maxBitrate, MAX_JITTER_MS);
    plotJitter.AddDataset(jitterThreshold);
    std::ofstream plotFileJitterOut(file_prefix_detailed + "_jitter_detallado.plt");
    plotJitter.GenerateOutput(plotFileJitterOut);
    plotFileJitterOut.close();
    ExecuteCommand("gnuplot " + file_prefix_detailed + "_jitter_detallado.plt");
    ExecuteCommand("rm -f " + file_prefix_detailed + "_jitter_detallado.plt");

    // Gráfica de Pérdida de Paquetes
    Gnuplot plotPerdida(file_prefix_detailed + "_perdida_detallada.png");
    plotPerdida.SetTitle("Perdida de Paquetes vs. Bitrate");
    plotPerdida.SetTerminal("pngcairo enhanced font 'Helvetica,12' size 800,600");
    // *** CORRECCIÓN: Se reemplaza el '%' por una palabra para evitar errores de Gnuplot. ***
    plotPerdida.SetLegend("Bitrate (Mbps)", "Perdida de Paquetes (porcentaje)");
    plotPerdida.SetExtra("set grid; set xtics 20;");
    for (auto const& [users, points] : processedDataByUserCount)
    {
        Gnuplot2dDataset dataset;
        dataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);
        dataset.SetTitle(std::to_string(users) + " Usuarios");
        for (const auto& point : points)
        {
            dataset.Add(point.bitrate, point.lossMean);
        }
        plotPerdida.AddDataset(dataset);
    }
    Gnuplot2dDataset lossThreshold;
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << MAX_LOSS_PERCENT;
    // *** CORRECCIÓN: Se reemplaza el '%' en el título del umbral. ***
    lossThreshold.SetTitle("Umbral (" + oss.str() + " porcentaje)");
    lossThreshold.SetStyle(Gnuplot2dDataset::LINES);
    lossThreshold.SetExtra("dt 2 lw 2");
    lossThreshold.Add(minBitrate, MAX_LOSS_PERCENT);
    lossThreshold.Add(maxBitrate, MAX_LOSS_PERCENT);
    plotPerdida.AddDataset(lossThreshold);
    std::ofstream plotFilePerdidaOut(file_prefix_detailed + "_perdida_detallada.plt");
    plotPerdida.GenerateOutput(plotFilePerdidaOut);
    plotFilePerdidaOut.close();
    ExecuteCommand("gnuplot " + file_prefix_detailed + "_perdida_detallada.plt");
    ExecuteCommand("rm -f " + file_prefix_detailed + "_perdida_detallada.plt");

    // --- GENERACIÓN DE GRÁFICAS DE PRECISIÓN ---
    std::string precision_prefix = std::string(precision_dir) + "/grafica_precision";

    // Gráfica de Precisión del Retardo
    Gnuplot plotDelayError(precision_prefix + "_retardo.png");
    plotDelayError.SetTitle("Precision del Retardo vs. Bitrate (Margen de Error 95%)");
    plotDelayError.SetTerminal("pngcairo enhanced font 'Helvetica,12' size 800,600");
    plotDelayError.SetLegend("Bitrate (Mbps)", "Margen de Error (+- ms)");
    plotDelayError.SetExtra("set grid; set xtics 20;");
    for (auto const& [users, points] : processedDataByUserCount)
    {
        Gnuplot2dDataset dataset;
        dataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);
        dataset.SetTitle(std::to_string(users) + " Usuarios");
        for (const auto& point : points)
        {
            dataset.Add(point.bitrate, point.delayMarginOfError);
        }
        plotDelayError.AddDataset(dataset);
    }
    std::ofstream plotFileDelayError(precision_prefix + "_retardo.plt");
    plotDelayError.GenerateOutput(plotFileDelayError);
    plotFileDelayError.close();
    ExecuteCommand("gnuplot " + precision_prefix + "_retardo.plt");
    ExecuteCommand("rm -f " + precision_prefix + "_retardo.plt");

    // Gráfica de Precisión del Jitter
    Gnuplot plotJitterError(precision_prefix + "_jitter.png");
    plotJitterError.SetTitle("Precision del Jitter vs. Bitrate (Margen de Error 95%)");
    plotJitterError.SetTerminal("pngcairo enhanced font 'Helvetica,12' size 800,600");
    plotJitterError.SetLegend("Bitrate (Mbps)", "Margen de Error (+- ms)");
    plotJitterError.SetExtra("set grid; set xtics 20;");
    for (auto const& [users, points] : processedDataByUserCount)
    {
        Gnuplot2dDataset dataset;
        dataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);
        dataset.SetTitle(std::to_string(users) + " Usuarios");
        for (const auto& point : points)
        {
            dataset.Add(point.bitrate, point.jitterMarginOfError);
        }
        plotJitterError.AddDataset(dataset);
    }
    std::ofstream plotFileJitterError(precision_prefix + "_jitter.plt");
    plotJitterError.GenerateOutput(plotFileJitterError);
    plotFileJitterError.close();
    ExecuteCommand("gnuplot " + precision_prefix + "_jitter.plt");
    ExecuteCommand("rm -f " + precision_prefix + "_jitter.plt");

    // Gráfica de Precisión de la Pérdida de Paquetes
    Gnuplot plotLossError(precision_prefix + "_perdida.png");
    plotLossError.SetTitle("Precision de la Perdida vs. Bitrate (Margen de Error 95)");
    plotLossError.SetTerminal("pngcairo enhanced font 'Helvetica,12' size 800,600");
    plotLossError.SetLegend("Bitrate (Mbps)", "Margen de Error (+- porcentaje)");
    plotLossError.SetExtra("set grid; set xtics 20;");
    for (auto const& [users, points] : processedDataByUserCount)
    {
        Gnuplot2dDataset dataset;
        dataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);
        dataset.SetTitle(std::to_string(users) + " Usuarios");
        for (const auto& point : points)
        {
            dataset.Add(point.bitrate, point.lossMarginOfError);
        }
        plotLossError.AddDataset(dataset);
    }
    std::ofstream plotFileLossError(precision_prefix + "_perdida.plt");
    plotLossError.GenerateOutput(plotFileLossError);
    plotFileLossError.close();
    ExecuteCommand("gnuplot " + precision_prefix + "_perdida.plt");
    ExecuteCommand("rm -f " + precision_prefix + "_perdida.plt");

    // --- GRÁFICA FINAL DE RESULTADOS ---
    std::string finalPlotFileName = std::string(dir) + "/grafica_ns3_final_bitrate_requerido.png";
    Gnuplot2dDataset finalDataset;
    finalDataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);
    finalDataset.SetTitle("Bitrate Requerido");
    finalDataset.SetExtra("lw 2 pt 7 ps 1.5");

    int validResultsCount = 0;
    std::cout << "\n--- Resumen para Gráfica Final (Intervalo de Confianza 95) ---\n";
    for (const auto& result : finalResults)
    {
        if (result.second.bitrate > 0)
        {
            finalDataset.Add(result.first, result.second.bitrate);
            validResultsCount++;
            std::cout << "Para " << result.first << " usuarios, Bitrate Min: " << result.second.bitrate
                      << " Mbps"
                      << " (Latencia: " << std::fixed << std::setprecision(2)
                      << result.second.delayMean << " +- " << result.second.delayMarginOfError
                      << " ms)" << std::endl;
        }
        else
        {
            std::cout << "Aviso: Para " << result.first
                      << " usuarios, ningún bitrate probado cumplió los requisitos de QoS."
                      << std::endl;
        }
    }
    std::cout << "----------------------------------------------------------------\n\n";

    if (validResultsCount > 0)
    {
        Gnuplot plotFinal(finalPlotFileName);
        plotFinal.SetTitle("Bitrate Minimo Requerido vs. Numero de Usuarios");
        plotFinal.SetTerminal("pngcairo enhanced font 'Helvetica,12' size 800,600");
        plotFinal.SetLegend("Numero de Usuarios", "Bitrate Minimo (Mbps)");
        plotFinal.SetExtra("set grid; set key top left; set xtics 50;");
        plotFinal.AddDataset(finalDataset);

        std::string pltPath = std::string(dir) + "/final.plt";
        std::ofstream plotFinalFile(pltPath);
        plotFinal.GenerateOutput(plotFinalFile);
        plotFinalFile.close();
        ExecuteCommand("gnuplot " + pltPath);
        ExecuteCommand("rm -f " + pltPath);

        std::cout << "Gráfica final generada correctamente: " << finalPlotFileName << std::endl;
    }
    else
    {
        std::cout << "AVISO: No se generó la gráfica final porque ningún punto cumplió los "
                     "requisitos de QoS."
                  << std::endl;
    }

    std::cout << "Proceso de generación de gráficas finalizado." << std::endl;

    return 0;
}
