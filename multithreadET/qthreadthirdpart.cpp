#include "multithreadET/qthreadthirdpart.h"
#include <QDebug>
#include <QVector>
#include <QStringList>
#include <QThread>

#include "registration/phase_correlation_function.h"
#include "registration/multiPOC_Ai1.h"
#include "image_analysis/image_processing.h"
#include "image_analysis/correlation_coefficient.h"
#include "util/files_folders_operations.h"

#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/highgui/highgui.hpp>
using cv::Mat;
using cv::Rect;

qThreadThirdPart::qThreadThirdPart(QStringList i_videoList,
                                   QVector<QString> i_badVideos,
                                   QMap<QString,QVector<int>> i_badFramesFirstEval,
                                   QMap<QString,QVector<int>> i_framesCompleteEval,
                                   QMap<QString, int> i_framesReferencial,
                                   QMap<QString, double> i_averageCC,
                                   QMap<QString, double> i_averageFWHM,
                                   QMap<QString,cv::Rect> i_standardCutout,
                                   QMap<QString,cv::Rect> i_extraCutout,
                                   bool i_scaleChange,
                                   double i_areaMaximum,
                                   QObject *parent):QThread(parent)
{
    videoList = i_videoList;
    badFrames_firstEvaluation = i_badFramesFirstEval;
    framesEvaluationCompelte = i_framesCompleteEval;
    referencialFrames = i_framesReferencial;
    obtainedCutoffStandard = i_standardCutout;
    obtainedCutoffExtra = i_extraCutout;
    scaleChanged = i_scaleChange;
    averageCCcomplete = i_averageCC;
    averageFWHMcomplete = i_averageFWHM;
    notProcessThese = i_badVideos;
    areaMaximum = i_areaMaximum;

    emit setTerminationEnabled(true);
}

void qThreadThirdPart::run()
{
    // Sixth part - using average values of FWHM and correlation coefficient, the bad frames are split into multiple
    // categories according to the their values of FWHM and correlation coefficient.

    emit typeOfMethod(2);
    emit percentageCompleted(0);
    videoCount = double(videoList.count());
    for (int videoIndex=0; videoIndex < videoList.count(); videoIndex++)
    {
        QString fullPath = videoList.at(videoIndex);
        QString folder,filename,suffix;
        processFilePath(fullPath,folder,filename,suffix);
        if (notProcessThese.indexOf(filename) == -1){
            QVector<int> _badFrames;
            _badFrames = badFrames_firstEvaluation[filename];


            emit actualVideo(videoIndex);
            cv::VideoCapture cap = cv::VideoCapture(fullPath.toLocal8Bit().constData());
            if (!cap.isOpened()){
                emit unexpectedTermination(videoIndex,"hardError");
                fillEmpty(filename,260);
                continue;
            }
            int videoFrameCount = int(cap.get(CV_CAP_PROP_FRAME_COUNT));
            cv::Mat referencialFrame_temp,referencialFrame,referencialFrame32f,referencialCutout;
            cap.set(CV_CAP_PROP_POS_FRAMES,referencialFrames[filename]);
            if (!cap.read(referencialFrame_temp))
            {
                emit unexpectedTermination(videoIndex,"hardError");
                fillEmpty(filename,videoFrameCount);
                continue;
            }
            int rows = 0;
            int cols = 0;
            cv::Rect _tempStandard,_tempExtra,_tempStandardAdjusted;
            _tempStandard = obtainedCutoffStandard[filename];
            _tempExtra = obtainedCutoffExtra[filename];

            if (scaleChanged == true)
            {
                referencialFrame_temp(_tempExtra).copyTo(referencialFrame);
                rows = referencialFrame.rows;
                cols = referencialFrame.cols;
                /*_tempStandardAdjusted = adjustStandardCutout(_tempExtra,
                                                             _tempStandard,
                                                             referencialFrame_temp.rows,
                                                             referencialFrame_temp.cols);*/
                referencialFrame(_tempStandard).copyTo(referencialCutout);
                referencialFrame_temp.release();
            }
            else
            {
                referencialFrame_temp.copyTo(referencialFrame);
                rows = referencialFrame.rows;
                cols = referencialFrame.cols;
                referencialFrame(_tempStandard).copyTo(referencialCutout);
                referencialFrame_temp.release();
            }

            QVector<double> pomVecD(videoFrameCount,0.0);
            QVector<double> POC_x,POC_y,_angle,frangi_x,frangi_y,frangi_euklid,_computedFWHM,_computedCC;
            frangi_x = pomVecD;frangi_y=pomVecD;frangi_euklid=pomVecD;
            POC_x=pomVecD;POC_y=pomVecD;_angle=pomVecD;
            QVector<int> _framesForFirstEvaluation;
            int badFrameCount = _badFrames.length();
            for (int i = 0;i < _badFrames.length(); i++)
            {
                emit percentageCompleted(qRound((double(videoIndex)/videoCount)*100.0+((double(i)/double(badFrameCount))*100.0)/videoCount));
                Mat moved_temp,moved,moved_cutOut,registrated,registrated_cutOut;
                Mat _frame, _frame_cutOut;
                referencialFrame.copyTo(_frame);
                _frame(_tempStandard).copyTo(_frame_cutOut);

                /*if (!scaleChanged)
                    _frame(_tempStandard).copyTo(_frame_cutOut);
                else
                    _frame(_tempStandardAdjusted).copyTo(_frame_cutOut);*/

                cap.set(CV_CAP_PROP_POS_FRAMES,_badFrames[i]);
                if (cap.read(moved_temp) != 1)
                {
                    unexpectedTermination(videoIndex,"hardError");
                    POC_x[badFrames_firstEvaluation[filename][i]] = 999.0;
                    POC_y[badFrames_firstEvaluation[filename][i]] = 999.0;
                    _angle[badFrames_firstEvaluation[filename][i]] = 999.0;
                    frangi_x[badFrames_firstEvaluation[filename][i]] = 999.0;
                    frangi_y[badFrames_firstEvaluation[filename][i]] = 999.0;
                    frangi_euklid[badFrames_firstEvaluation[filename][i]] = 999.0;
                    fillEmpty(filename,videoFrameCount);
                    continue;
                }

                if (scaleChanged == true)
                {
                    moved_temp(_tempExtra).copyTo(moved);
                    moved(_tempStandard).copyTo(moved_cutOut);
                    moved_temp.release();
                }
                else
                {
                    moved_temp.copyTo(moved);
                    moved(_tempStandard).copyTo(moved_cutOut);
                    moved_temp.release();
                }

                cv::Point3d pt(0,0,0);
                //if (scaleChanged == true)
                //{
                    pt = pc_translation_hann(_frame,moved,areaMaximum);
                    if (std::abs(pt.x)>=290 || std::abs(pt.y)>=290)
                    {
                        pt = pc_translation(_frame,moved,areaMaximum);
                    }
                //}
                /*if (scaleChanged == false)
                {
                    pt = pc_translation_hann(_frame,moved,areaMaximum);
                }*/
                if (pt.x >= 55 || pt.y >= 55)
                {
                    qDebug()<< "Frame "<< _badFrames[i]<< " will not be analysed.";
                    framesEvaluationCompelte[filename][_badFrames[i]] = 5;
                    POC_x[badFrames_firstEvaluation[filename][i]] = 999;
                    POC_y[badFrames_firstEvaluation[filename][i]] = 999;
                    _angle[badFrames_firstEvaluation[filename][i]] = 999;
                    frangi_x[badFrames_firstEvaluation[filename][i]] = 999;
                    frangi_y[badFrames_firstEvaluation[filename][i]] = 999;
                    frangi_euklid[badFrames_firstEvaluation[filename][i]] = 999;
                    moved.release();
                }
                else
                {
                    double sigma_gauss = 1/(std::sqrt(2*CV_PI)*pt.z);
                    double FWHM = 2*std::sqrt(2*std::log(2)) * sigma_gauss;
                    registrated = frameTranslation(moved,pt,rows,cols);
                    double R = -1;
                    registrated(_tempStandard).copyTo(registrated_cutOut);
                    R = calculateCorrCoef(_frame,registrated,_tempStandard);
                    /*if (!scaleChanged){
                        registrated(_tempStandard).copyTo(registrated_cutOut);
                        R = calculateCorrCoef(_frame,registrated,_tempStandard);
                    }
                    else{
                        registrated(_tempStandardAdjusted).copyTo(registrated_cutOut);
                        R = calculateCorrCoef(_frame,registrated,_tempStandardAdjusted);
                    }*/

                    qDebug() <<"Tested frame has "<<_badFrames[i]<< "R " << R <<" a FWHM " << FWHM;
                    registrated.release();
                    registrated_cutOut.release();
                    moved.release();
                    double _differenceCC = averageCCcomplete[filename]-R;
                    double _differenceFWHM = averageFWHMcomplete[filename]-FWHM;
                    if ((std::abs(_differenceCC) < 0.02) && (FWHM < averageFWHMcomplete[filename])) //1.
                    {
                        qDebug()<< "Frame "<< _badFrames[i]<< " ready for registration.";
                        framesEvaluationCompelte[filename][badFrames_firstEvaluation[filename][i]] = 0.0;
                        continue;
                    }
                    else if (R > averageCCcomplete[filename] && (std::abs(_differenceFWHM)<=2||(FWHM < averageFWHMcomplete[filename]))) //5.
                    {
                        qDebug()<< "Frame "<< _badFrames[i]<< " ready for registration.";
                        framesEvaluationCompelte[filename][badFrames_firstEvaluation[filename][i]] = 0.0;
                        continue;
                    }
                    else if (R >= averageCCcomplete[filename] && FWHM > averageFWHMcomplete[filename]) //4.
                    {
                        qDebug()<< "Frame "<< _badFrames[i]<< " ready for registration.";
                        framesEvaluationCompelte[filename][badFrames_firstEvaluation[filename][i]] = 0.0;
                        continue;
                    }
                    else if ((std::abs(_differenceCC) <= 0.02) && (FWHM > averageFWHMcomplete[filename])) //2.
                    {
                        qDebug()<< "Frame "<< _badFrames[i]<< " will be analysed in the next step.";
                        _framesForFirstEvaluation.push_back(badFrames_firstEvaluation[filename][i]);
                        _computedFWHM.push_back(FWHM);
                        _computedCC.push_back(R);
                        continue;
                    }
                    else if ((_differenceCC > 0.02) && (_differenceCC < 0.18)) //3.
                    {
                        qDebug()<< "Frame "<< _badFrames[i]<< " will be analysed in the next step.";
                        _framesForFirstEvaluation.push_back(badFrames_firstEvaluation[filename][i]);
                        _computedFWHM.push_back(FWHM);
                        _computedCC.push_back(R);
                        continue;
                    }
                    else if ((_differenceCC >= 0.05 && _differenceCC < 0.18) && ((FWHM < averageFWHMcomplete[filename]) || averageFWHMcomplete[filename] > 35.0)) //6.
                    {
                        qDebug()<< "Frame "<< _badFrames[i]<< " will be analysed in the next step.";
                        _framesForFirstEvaluation.push_back(badFrames_firstEvaluation[filename][i]);
                        _computedFWHM.push_back(FWHM);
                        _computedCC.push_back(R);
                        continue;
                    }
                    else if ((_differenceCC >= 0.05 && _differenceCC < 0.18) && (FWHM <= (averageFWHMcomplete[filename]+10))) //8.
                    {
                        qDebug()<< "Frame "<< _badFrames[i]<< " will be analysed in the next step.";
                        _framesForFirstEvaluation.push_back(badFrames_firstEvaluation[filename][i]);
                        _computedFWHM.push_back(FWHM);
                        _computedCC.push_back(R);
                        continue;
                    }

                    else if ((_differenceCC >= 0.2) && (FWHM > (averageFWHMcomplete[filename]+10))) //7.
                    {
                        qDebug()<< "Frame "<< _badFrames[i]<< " will not be registrated.";
                        framesEvaluationCompelte[filename][badFrames_firstEvaluation[filename][i]] = 5;
                        POC_x[badFrames_firstEvaluation[filename][i]] = 999;
                        POC_y[badFrames_firstEvaluation[filename][i]] = 999;
                        _angle[badFrames_firstEvaluation[filename][i]] = 999;
                        frangi_x[badFrames_firstEvaluation[filename][i]] = 999;
                        frangi_y[badFrames_firstEvaluation[filename][i]] = 999;
                        frangi_euklid[badFrames_firstEvaluation[filename][i]] = 999;
                        continue;
                    }
                    else
                    {
                        qDebug() << "Frame "<< _badFrames[i]<< " will be analysed in the next step.";
                        _framesForFirstEvaluation.push_back(badFrames_firstEvaluation[filename][i]);
                        _computedFWHM.push_back(FWHM);
                        _computedCC.push_back(R);

                    }
                }

            }
            framesComputedCC.insert(filename,_computedCC);
            framesComputedFWHM.insert(filename,_computedFWHM);
            framesFirstCompleteEvaluation.insert(filename,_framesForFirstEvaluation);
            framesFrangiX.insert(filename,frangi_x);
            framesFrangiY.insert(filename,frangi_y);
            framesFrangiEuklid.insert(filename,frangi_euklid);
            framesPOCX.insert(filename,POC_x);
            framesPOCY.insert(filename,POC_y);
            framesUhel.insert(filename,_angle);
        }
        else{
            fillEmpty(filename,260);
        }
    }
    emit percentageCompleted(100);
    emit done(3);
}

void qThreadThirdPart::fillEmpty(QString i_videoName, int i_frameCount){
    /*QVector<double> pomVecD(i_frameCount,0.0);
    QVector<int> pomVecI(i_frameCount,0);

    framesEvaluationCompelte.insert(i_videoName,pomVecI);
    framesFirstCompleteEvaluation.insert(i_videoName,pomVecI);
    framesComputedCC.insert(i_videoName,pomVecD);
    framesComputedFWHM.insert(i_videoName,pomVecD);
    framesFrangiX.insert(i_videoName,pomVecD);
    framesFrangiY.insert(i_videoName,pomVecD);
    framesFrangiEuklid.insert(i_videoName,pomVecD);
    framesPOCX.insert(i_videoName,pomVecD);
    framesPOCY.insert(i_videoName,pomVecD);
    framesUhel.insert(i_videoName,pomVecD);*/
    notProcessThese.push_back(i_videoName);
}

QVector<QString> qThreadThirdPart::unprocessableVideos() {
    return notProcessThese;
}

QMap<QString, QVector<int> > qThreadThirdPart::framesUpdateEvaluation()
{
    return framesEvaluationCompelte;
}
QMap<QString, QVector<int> > qThreadThirdPart::framesFirstEvaluationComplete()
{
    return framesFirstCompleteEvaluation;
}
QMap<QString,QVector<double>> qThreadThirdPart::framesProblematic_CC()
{
    return framesComputedCC;
}
QMap<QString, QVector<double> > qThreadThirdPart::framesProblematic_FWHM()
{
    return framesComputedFWHM;
}
QMap<QString,QVector<double>> qThreadThirdPart::framesFrangiXestimated()
{
    return framesFrangiX;
}
QMap<QString,QVector<double>> qThreadThirdPart::framesFrangiYestimated()
{
    return framesFrangiY;
}
QMap<QString,QVector<double>> qThreadThirdPart::framesFrangiEuklidestimated()
{
    return framesFrangiEuklid;
}
QMap<QString,QVector<double>> qThreadThirdPart::framesPOCXestimated()
{
    return framesPOCX;
}
QMap<QString,QVector<double>> qThreadThirdPart::framesPOCYestimated()
{
    return framesPOCY;
}
QMap<QString,QVector<double>> qThreadThirdPart::framesUhelestimated()
{
    return framesUhel;
}

void qThreadThirdPart::onDataObtained(){
    emit readyForFinish();
}
