#include "learnwindows.h"
#include <QDockWidget>
#include <QFileDialog>
#include <QVector>
#include <QTextStream>

IplImage *src_img, *active_img = NULL;
bool drawing_box = false;
CvRect box;
CvPoint start = cvPoint(-1,-1);

QVector<CvRect> windowRegions;
QVector<CvRect> nonRegions;

#define cvImage IplImage*

QString fileName;
QStringList filesFolder;
QString currDirectory;

bool isOpenCVReady = false;
bool isShowNon = true;
bool isExportInfo = true;

LearnWindows::LearnWindows(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);

	//QDockWidget * mainDock = new QDockWidget("Work area");
	//mainDock->setWidget (ui.workArea);
	//ui.mainWidget->layout()->addWidget(mainDock);

	connect(ui.actionOpenImage, SIGNAL(triggered()), SLOT(OpenImage()));
	connect(ui.saveWindowsButton, SIGNAL(clicked()), SLOT(SaveAllRegions()));
	connect(ui.nextImageButton, SIGNAL(clicked()), SLOT(NextImage()));
	connect(ui.prevImageButton, SIGNAL(clicked()), SLOT(PrevImage()));
	connect(ui.clearRegionsButton, SIGNAL(clicked()), SLOT(ClearRegions()));

	connect(ui.undoWindowRegion, SIGNAL(clicked()), SLOT(UndoWindowRegion()));
	connect(ui.undoNonRegion, SIGNAL(clicked()), SLOT(UndoNonRegion()));

	connect(ui.exportSamplesButton, SIGNAL(clicked()), SLOT(ExportAllSamples()));

	connect(ui.toggleNon, SIGNAL(clicked()), SLOT(ToggleNon()));
	connect(ui.sampleBackgroundsButton, SIGNAL(clicked()), SLOT(ExportBackgroundSamples()));

	this->show();

	// Create OpenCV window
	cvNamedWindow("Window1");
	//cvNamedWindow("Preview");

	// Set mouse handling
	cvSetMouseCallback("Window1", handleMouse, (void*) src_img);

	// Position OpenCV window
	cvMoveWindow("Window1", this->x() + this->width() + 15, this->y());

	isOpenCVReady = true;
}

LearnWindows::~LearnWindows()
{

}

void LearnWindows::OpenImage()
{
	QString filename = QFileDialog::getOpenFileName(0, "Open image", "", "Image Files (*.png *.jpg *.bmp *.gif)"); 

	if(filename.size())
	{
		// Setup images
		LoadImage(filename);

		// Load saved regions
		LoadAllRegions();

		// Keep track of files in this folder
		currDirectory = filename.left(filename.lastIndexOf("/"));
		filesFolder = QDir(currDirectory).entryList().filter(QRegExp("^.*\\.(png|jpg|bmp|gif)$"));
	}
}

void LearnWindows::LoadImage(QString filename)
{
	IplImage * img = cvLoadImage(qPrintable(filename));
	IplImage * out = cvCreateImage(cvSize( img->width * 0.5, img->height * 0.5 ), img->depth, img->nChannels);
	cvResize(img, out);
	cvShowImage("Window1", out);
	src_img = out;

	fileName = filename;

	// Preprocessing:
	//cvCanny( img, img, 10, 100, 3 );
	//IplImage *corners = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_32F, 1);
	//cvPreCornerDetect( img, corners, 7 );
	//cvSmooth(img, img, CV_BLUR, 1, 10);
	//cvEqualizeHist(out, out);
}

void LearnWindows::SaveAllRegions()
{
	SaveRegions("seg", windowRegions);
	SaveRegions("non", nonRegions);
}

void LearnWindows::LoadAllRegions()
{
	LoadRegions("seg", windowRegions);
	LoadRegions("non", nonRegions);

	redrawRegions();
}

void LearnWindows::SaveRegions(QString ext, QVector<CvRect> regions)
{
	if(!regions.size()) return;

	QString outFileName = fileName; 
	outFileName.chop(3); outFileName += ext;

	QFile file(outFileName);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return;

	QTextStream out(&file);

	foreach(CvRect rect, regions)
	{
		double x = double(rect.x) / src_img->width;
		double y = double(rect.y) / src_img->height;
		double w = double(rect.width) / src_img->width;
		double h = double(rect.height) / src_img->height;

		out << x << " " << y << " " << w << " " << h << "\n";
	}

	file.close();
}

void LearnWindows::LoadRegions( QString ext, QVector<CvRect> & regions )
{
	double w = src_img->width;
	double h = src_img->height;

	QString outFileName	= fileName; 

	outFileName.chop(3); outFileName += ext;

	regions.clear();

	QFile file(outFileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;


	QTextStream in(&file);
	while (!in.atEnd()) {
		QStringList line = in.readLine().split(" ");
		QVector<double> v;

		foreach(QString item, line)
			v.push_back(item.toDouble());

		regions.push_back(cvRect(v[0] * w, v[1] * h, v[2] * w, v[3] * h));
	}
	file.close();
}

void LearnWindows::ClearRegions()
{
	windowRegions.clear();
	nonRegions.clear();
	redrawRegions();
}

void LearnWindows::NextImage()
{
	SaveAllRegions();
	QString f = QFileInfo(fileName).fileName();
	if(!f.size())	return;
	int i = NEXT(filesFolder.indexOf(f), filesFolder.size());
	LoadImage(currDirectory + "/" + filesFolder[i]);
	LoadAllRegions();
}

void LearnWindows::PrevImage()
{
	SaveAllRegions();
	QString f = QFileInfo(fileName).fileName();
	if(!f.size())	return;
	int i = PREV(filesFolder.indexOf(f), filesFolder.size());
	LoadImage(currDirectory + "/" + filesFolder[i]);
	LoadAllRegions();
}

void LearnWindows::UndoWindowRegion()
{
	if(windowRegions.size())
		windowRegions.remove(windowRegions.size()-1);
	SaveRegions("seg", windowRegions);
	redrawRegions();
}

void LearnWindows::UndoNonRegion()
{
	if(nonRegions.size())
		nonRegions.remove(nonRegions.size()-1);
	SaveRegions("non", nonRegions);
	redrawRegions();
}

void LearnWindows::moveEvent( QMoveEvent * ee )
{
	// Position OpenCV window
	if(isOpenCVReady)
		cvMoveWindow("Window1", this->x() + this->width() + 15, this->y());
}

IplImage * clone(IplImage * fromImg)
{
	if(!fromImg) return NULL;

	IplImage * result = cvCreateImage(cvSize(fromImg->width, fromImg->height), fromImg->depth, fromImg->nChannels);
	cvCopy(fromImg, result);
	return result;
}

IplImage * cloneSrc()
{
	return clone(src_img);
}

IplImage * emptyClone(IplImage * fromImg)
{
	IplImage * result = cvCreateImage(cvSize(fromImg->width, fromImg->height), fromImg->depth, fromImg->nChannels);
	return result;
}

void drawBox( IplImage* img, CvRect rect, int r, int g, int b )
{
	// Special case
	if((r == g) && (g == b))
		cvRectangle( img, cvPoint(rect.x, rect.y), cvPoint(rect.x+rect.width,rect.y+rect.height), cvScalar(0,0,0), 3);

	cvRectangle( img, cvPoint(rect.x, rect.y), cvPoint(rect.x+rect.width,rect.y+rect.height), cvScalar(b,g,r), 2);
}

void handleMouse( int event, int x, int y, int flags, void* param )
{
	switch( event ){
		case CV_EVENT_MOUSEMOVE: 
			if( drawing_box ){
				box.width = x-box.x;
				box.height = y-box.y;

				active_img = cloneSrc();
				drawBox(active_img, cvRect(start.x, start.y, box.width, box.height));
				cvShowImage("Window1", active_img);
				cvReleaseImage(&active_img);
			}
			break;

		case CV_EVENT_LBUTTONDOWN:
			drawing_box = true;
			box = cvRect( x, y, 0, 0 );
			if(start.x < 0)
				start = cvPoint(x,y);
			break;

		case CV_EVENT_LBUTTONUP:
			drawing_box = false;
			start = cvPoint(-1,-1);

			if( box.width < 0 ){
				box.x += box.width;
				box.width *= -1;
			}
			if( box.height < 0 ){
				box.y += box.height;
				box.height *= -1;
			}

			if(flags & CV_EVENT_FLAG_CTRLKEY)
			{
				// add a non region of fixed size, for ease of use
				CvRect b;
				int windowSize = (int)(0.15 * max(src_img->width, src_img->height));
				int halfWindow = 0.5 * windowSize;
				b.x = max(0, min(src_img->width - (windowSize + 1), x - halfWindow));
				b.y = max(0, min(src_img->height - (windowSize + 1), y - halfWindow));
				b.width = windowSize;
				b.height = windowSize;
				nonRegions.push_back(b);
			}
			else
			{
				if(box.width < 10 || box.height < 10) break;

				// Add region
				if(flags & CV_EVENT_FLAG_SHIFTKEY)
					nonRegions.push_back(box);
				else
					windowRegions.push_back(box);
			}

			// Draw regions and show it
			redrawRegions();

			break;
	}
}

void redrawRegions()
{
	IplImage * cur_img = cloneSrc();

	if(!cur_img) return;

	foreach(CvRect rect, windowRegions) drawBox(cur_img, rect, 0, 0, 255);
	if(isShowNon)foreach(CvRect rect, nonRegions) drawBox(cur_img, rect, 255, 0, 0);

	cvShowImage("Window1", cur_img);

	cvReleaseImage(&cur_img); // clean up
}

void LearnWindows::ExportAllSamples()
{
	if(!filesFolder.size()) return;

	// Export processed images
	int maxDimension = 320;

	QString exportDir = currDirectory + "/export";
	QDir().mkdir(exportDir);

	foreach(QString filename, filesFolder)
	{
		QString src_filename = currDirectory + "/" + filename;
		IplImage * img = cvLoadImage(qPrintable(src_filename));

		// RESIZE:
		//double scaling = maxDimension / double(max(img->width, img->height));
		double scaling = 1.0;
		IplImage * scaled = cvCreateImage(cvSize( img->width * scaling, img->height * scaling ), img->depth, img->nChannels);
		cvResize(img, scaled, CV_INTER_CUBIC );

		// GRAYSCALE
		IplImage *grayImg = cvCreateImage( cvSize( scaled->width, scaled->height ), IPL_DEPTH_8U, 1 );
		cvCvtColor( scaled, grayImg, CV_RGB2GRAY );

		// EXPORT final to new file
		QString fileIndex =  QString::number(filesFolder.indexOf(filename)).rightJustified(3, '0');
		QString exportFileName = QString(exportDir + "/%1%2.bmp").arg(ui.exportTitle->text()).arg(fileIndex);
		if(QFileInfo(exportFileName).exists()) QFile::remove(exportFileName);

		cvSaveImage(qPrintable(exportFileName), grayImg);
		
		// Clean up
		cvReleaseImage(&img);
		cvReleaseImage(&scaled);
		cvReleaseImage(&grayImg);	
	}

	// Export info files
	if(isExportInfo)
	{
		if(windowRegions.size())	ExportSamplesRegion("seg");
		if(nonRegions.size())	ExportSamplesRegion("non");
	}

	isExportInfo = true;
}

void LearnWindows::ExportSamplesRegion( QString ext )
{
	QString exportDir = currDirectory + "/export";

	QFile file(exportDir + "/" + ui.exportTitle->text() + "_" + ext + ".txt");
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))	return;
	QTextStream out(&file);

	foreach(QString filename, filesFolder)
	{
		QVector< QVector<double> > currRegions;

		// Load regions from 'ext' file
		QString orginalFile = filename;
		filename.chop(3); filename += ext;
		QString regionFile = currDirectory + "/" + filename;
		QFile file(regionFile);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
		QTextStream in(&file);
		while (!in.atEnd()) {
			QVector<double> v;QStringList line = in.readLine().split(" ");
			foreach(QString item, line) v.push_back(item.toDouble());

			currRegions.push_back(v);
		}file.close();

		if(currRegions.isEmpty()) continue;

		QString fileIndex =  QString::number(filesFolder.indexOf(orginalFile)).rightJustified(3, '0');
		QString exportedFileName = QString("%1%2.bmp").arg(ui.exportTitle->text()).arg(fileIndex);
		QString exportedFilePath = exportDir + "/" + exportedFileName;

		// Write sample file name and number of marked regions in sample
		out << exportedFileName << " " << currRegions.size();

		// Get width and height of exported sample
		cvImage img = cvLoadImage(qPrintable(exportedFilePath));
		double width = img->width, height = img->height;
		int x,y,w,h;

		foreach(QVector<double> v, currRegions)
		{
			x = width * v[0]; y = height * v[1];
			w = width * v[2]; h = height * v[3];

			// Output region info
			out << " " << x << " " << y << " " << w << " " << h;
		}

		out << "\n";

		// Debug: output region
		/*QString previewFile = exportDir + "/" + QString("lastRegion.bmp");
		IplImage * preview = cvCreateImage(cvSize( w,h ), img->depth, img->nChannels);
		cvSetImageROI( img,  cvRect( x,y, w,h ) );
		cvCopy(img, preview);
		if(QFileInfo(previewFile).exists()) QFile::remove(previewFile);
		cvSaveImage(qPrintable(previewFile), preview);
		cvShowImage("Preview", preview);*/

		// Clean up
		cvReleaseImage(&img);
	}

	file.close();
}

void LearnWindows::ToggleNon()
{
	isShowNon = !isShowNon;
	redrawRegions();
}

void LearnWindows::ExportBackgroundSamples()
{
	// Export processed images
	isExportInfo = false;
	ExportAllSamples();

	QString exportDir = currDirectory + "/export";
	QFile file(exportDir + "/" + ui.exportTitle->text() + "_non.txt");
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))	return;
	QTextStream out(&file);

	// Create samples by sampling the image uniformly
	foreach(QString filename, filesFolder)
	{
		QString fileIndex =  QString::number(filesFolder.indexOf(filename)).rightJustified(3, '0');
		QString exportedFileName = QString("%1%2.bmp").arg(ui.exportTitle->text()).arg(fileIndex);
		QString exportedFilePath = exportDir + "/" + exportedFileName;

		// Load image to be sampled
		cvImage img = cvLoadImage(qPrintable(exportedFilePath));

		QVector< QVector<double> > currRegions;

		int windowWidth = double(img->width) * 0.08;
		int windowHeight = double(img->width) * 0.12;
		
		for(int i = 1; i < img->width - windowWidth; i += windowWidth )
		{
			for(int j = 1; j < img->height - windowHeight; j += windowHeight )
			{
				QVector<double> v(4);
				
				v[0] = i; v[1] = j;
				v[2] = windowWidth; v[3] = windowHeight;

				currRegions.push_back(v);
			}	
		}

		// Write sample file name and number of marked regions in sample
		out << exportedFileName << " " << currRegions.size();

		// Get width and height of exported sample
		double width = img->width, height = img->height;
		int x,y,w,h;

		foreach(QVector<double> v, currRegions){
			x = v[0]; y = v[1];
			w = v[2]; h = v[3];

			// Output region info
			out << " " << x << " " << y << " " << w << " " << h;
		}

		out << "\n";
	}
	
	file.close();
}
