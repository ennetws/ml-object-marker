#ifndef LEARNWINDOWS_H
#define LEARNWINDOWS_H

#include <QtGui/QMainWindow>
#include "ui_learnwindows.h"

// OpenCV stuff
#include "opencv2/core/core.hpp"
#include <opencv2/imgproc/imgproc_c.h>
#include "opencv2/highgui/highgui.hpp"
#include <stdio.h>
using namespace cv;

class LearnWindows : public QMainWindow
{
	Q_OBJECT

public:
	LearnWindows(QWidget *parent = 0, Qt::WFlags flags = 0);
	~LearnWindows();

	void moveEvent ( QMoveEvent * );

public slots:
	void OpenImage();
	void LoadImage(QString);
	void SaveAllRegions();
	void LoadAllRegions();
	void NextImage();
	void PrevImage();
	void ClearRegions();

	void SaveRegions( QString ext, QVector<CvRect> regions );
	void LoadRegions( QString ext, QVector<CvRect> & regions );
	void UndoWindowRegion();
	void UndoNonRegion();

	void ExportAllSamples();
	void ExportSamplesRegion(QString ext);
	void ExportBackgroundSamples();

	void ToggleNon();

private:
	Ui::LearnWindowsClass ui;
};

// Low level style... :(
void handleMouse( int event, int x, int y, int flags, void* param );
void drawBox( IplImage* img, CvRect rect, int r = 255, int g = 255, int b = 255);
void redrawRegions();

IplImage * clone(IplImage * fromImg);
IplImage * cloneSrc();
IplImage * emptyClone(IplImage * fromImg);

#define PREV(i, N) ((i + N-1) % N)
#define NEXT(i, N) ((i + 1) % N)

#endif // LEARNWINDOWS_H
