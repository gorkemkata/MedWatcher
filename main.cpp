#include <opencv\cv.h>
#include <opencv\highgui.h>
#include <string>

#include <CSmtp.h>
#include <iostream>

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>



// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib !
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "6000"
using namespace std;
using namespace cv;

static int SENSITIVITY_VALUE1 = 20;
static int SENSITIVITY_VALUE2 = 24;
static int BLUR_SIZE = 19;
static int MAX_MOTION_COUNTER = 3;
static bool objectDetected;
static Mat temp;
static int x;
static int y;
static int w;
static int h;
static int minW = 50;
static int maxW = 450;
static int minH = 180;
static int maxH = 450;
static int fallHeight = 100;
static int motionCounter = 0;
static vector<int> center(2);
static vector<int> oldCenter(2);
static int maxCenter = 1000;
static int difX;
static int difY;
static int fallInverval = 0;
static Rect objectBoundingRectangle = Rect(0, 0, 0, 0);
static std::string ipAddress;
static std::string eMailAddress;
static std::string path;
static std::string message = "patient fell";
static CSmtp mail;
static boolean vectorsNull = true;
static int videoFrameCount = 0;
static boolean capturingFallingVideo = false;
static int fallCount = 0;

static vector< vector<Point> > contours;
static vector<Vec4i> hierarchy;
static vector< vector<Point> > largestContourVec;

void searchForMovement(Mat thresholdImage, Mat &cameraFeed){

	objectDetected = false;
	thresholdImage.copyTo(temp);
	
	largestContourVec.clear();
	hierarchy.clear();



	findContours(temp, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
	vectorsNull = false;

	if (motionCounter < 0)
		motionCounter = 0;
	if (motionCounter > MAX_MOTION_COUNTER)
		motionCounter = MAX_MOTION_COUNTER;
	if (motionCounter > 0)
		putText(cameraFeed, "MOTION DETECTED", Point(40, 40), 1, 2, Scalar(0, 0, 255), 3);
	else
		putText(cameraFeed, "NO MOTION", Point(40, 40), 1, 2, Scalar(0, 0, 255), 3);
	

	if (contours.size()>0)
		objectDetected = true;
	else
		objectDetected = false;

	//cout << "MOTION COUNTER: " << motionCounter << "\n";

	if (objectDetected){
		largestContourVec.push_back(contours.at(contours.size() - 1));
		objectBoundingRectangle = boundingRect(largestContourVec.at(0));
		x = objectBoundingRectangle.x;
		y = objectBoundingRectangle.y;
		w = objectBoundingRectangle.width;
		h = objectBoundingRectangle.height;
		//	cout << "BOUNDING BOX WIDTH: " << w << "\n";
		if (w > minW && w < maxW && h > minH && h < maxH){
			line(cameraFeed, Point(x, y), Point(x + w, y), Scalar(0, 255, 0), 2);
			line(cameraFeed, Point(x + w, y), Point(x + w, y + h), Scalar(0, 255, 0), 2);
			line(cameraFeed, Point(x + w, y + h), Point(x, y + h), Scalar(0, 255, 0), 2);
			line(cameraFeed, Point(x, y + h), Point(x, y), Scalar(0, 255, 0), 2);
			circle(cameraFeed, Point(x + (w / 2), y + (h / 2)), 5, Scalar(0, 0, 255), 5);
			center.push_back(x + int(w / 2));
			center.push_back(y + int(h / 2));
			motionCounter++;
			//cout << "COORDINATES-->  X : " << center.at(0) << " WIDTH: " << w << endl;
			//cout << "COORDINATES-->  Y : " << center.at(1) << " HEIGHT: " << h << endl;
			//cout << "---------------------------\n";
		}
		else
		{
			center.push_back(0);
			center.push_back(0);
		}
	}
	else
	{
		//	cout << "There is no moving object!!! \n";
		center.push_back(0);
		center.push_back(0);
		motionCounter--;
	}
		
	
}

void sendMessage(string ip, string message)
{
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	const	char *sendbuf = message.c_str();
	const	char *serverIp = ip.c_str();
	char recvbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);


	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port 
	iResult = getaddrinfo(serverIp, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();

	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();


		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();

	}

	// Send an initial buffer
	iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
	if (iResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();


	}

	printf("Bytes Sent: %ld\n", iResult);

	// shutdown the connection since no more data will be sent
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();


	}

	// Receive until the peer closes the connection
	do {

		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
			printf("Bytes received: %d\n", iResult);
		else if (iResult == 0)
			printf("Connection closed\n");

		int count = 0;

		while (count < iResult){
			printf("%c", recvbuf[count]);

			count++;
		}


	} while (iResult > 0);

	// cleanup
	closesocket(ConnectSocket);
	WSACleanup();


}

void sendMail(String mailAddress, string message){
	bool bError = false;

	try
	{

#define test_gmail_tls

#if defined(test_gmail_tls)
		mail.SetSMTPServer("smtp.gmail.com", 587);
		mail.SetSecurityType(USE_TLS);
#elif defined(test_gmail_ssl)
		mail.SetSMTPServer("smtp.gmail.com", 465);
		mail.SetSecurityType(USE_SSL);
#elif defined(test_hotmail_TLS)
		mail.SetSMTPServer("smtp.live.com", 25);
		mail.SetSecurityType(USE_TLS);
#elif defined(test_aol_tls)
		mail.SetSMTPServer("smtp.aol.com", 587);
		mail.SetSecurityType(USE_TLS);
#elif defined(test_yahoo_ssl)
		mail.SetSMTPServer("plus.smtp.mail.yahoo.com", 465);
		mail.SetSecurityType(USE_SSL);
#endif

		mail.SetLogin("*************@gmail.com");
		mail.SetPassword("*************");
		mail.SetSenderName("MedWatcher");
		mail.SetSenderMail("*************@gmail.com");
		mail.SetReplyTo("*************");
		mail.SetSubject("Message From The Watcher");
		mail.AddRecipient(mailAddress.c_str());
		mail.SetXPriority(XPRIORITY_NORMAL);
		mail.SetXMailer("The Bat! (v3.02) Professional");
		mail.AddMsgLine("Uyari:");
		mail.AddMsgLine("");
		mail.AddMsgLine("");
		mail.AddMsgLine(message.c_str());
		mail.AddMsgLine("");
		mail.AddMsgLine("Umariz her sey yolundadir.");
		mail.AddMsgLine("");
		mail.AddMsgLine("MedWatcher");

		//mail.AddAttachment("../test1.jpg");
		//mail.AddAttachment("c:\\test2.exe");
		std::cout << "Adding Attachment \n";
		path = ".\\out" + to_string(fallCount) + ".avi";
		mail.AddAttachment(path.c_str());
		std::cout << "Attachment added \n";
		std::cout << "Sending Mail. Please wait... \n";
		mail.Send();

	}
	catch (ECSmtp e)
	{
		std::cout << "Error: " << e.GetErrorText().c_str() << ".\n";
		bError = true;
	}
	if (!bError)
		std::cout << "Mail was send successfully.\n";



}

int main(){
	bool isPaused = false;
	Mat frame1, frame2;
	Mat grayImage1, grayImage2;
	Mat differenceImage;
	Mat thresholdImage;
	VideoCapture videoCapture;

	videoCapture.open(0);
	int frameCount = 0;
	bool didFall = false;

	if (!videoCapture.isOpened()){
		cout << "Webcam is not found!!\n";
		getchar();
		return 1;
	}

	int frame_width = videoCapture.get(CV_CAP_PROP_FRAME_WIDTH);
	int frame_height = videoCapture.get(CV_CAP_PROP_FRAME_HEIGHT);
	VideoWriter videoWriter("out.avi", CV_FOURCC('M', 'J', 'P', 'G'), 10, Size(frame_width, frame_height), true);


	string line;
	ifstream configFileIn("config.cfg");
	if (configFileIn.is_open())
	{
		while (getline(configFileIn, line))
		{
			eMailAddress = line;
		}
		configFileIn.close();
	}
	else{
		cout << "Enter your email: \n";
		cin >> eMailAddress;

		ofstream configFileOut("config.cfg");
		if (configFileOut.is_open())
		{
			configFileOut << eMailAddress;
			configFileOut.close();
		}

	}

	cout << "COMMANDS: \n";
	cout << "Press E to set Object Sizes\n";
	cout << "Press S to set Sensivity values\n";
	cout << "Press B to set blur\n";
	cout << "Press A to set fall interval\n";
	cout << "Press C to set fall height\n";
	cout << "Press F to set max motion counter\n";
	cout << "Press D to set default values\n";
	cout << "Press Spacebar to pause\n";
	cout << "Press Esc to quit\n";

	while (1){

		videoCapture.read(frame1);

		//cout << "\nframe count: " << videoFrameCount;

		videoWriter.write(frame1);

		
		videoFrameCount++;

		if ( !capturingFallingVideo && videoFrameCount > 40){
			videoWriter.release();
			videoWriter.open("out" + to_string(fallCount) + ".avi", CV_FOURCC('M', 'J', 'P', 'G'), 10, Size(frame_width, frame_height), true);
			videoFrameCount = 0;
		}

			



		if (!frame1.empty()){
			//cv::imshow("Original Feed Frame 1", frame1);
			cv::cvtColor(frame1, grayImage1, COLOR_BGR2GRAY);
		}
		videoCapture.read(frame2);
		if (!frame2.empty()){
			//cv::imshow("Original Feed Frame 2", frame2);
			cv::cvtColor(frame2, grayImage2, COLOR_BGR2GRAY);
			cv::absdiff(grayImage1, grayImage2, differenceImage);
			cv::threshold(differenceImage, thresholdImage, SENSITIVITY_VALUE1, 255, THRESH_BINARY);
			//cv::imshow("absdiff", differenceImage);
			//cv::imshow("Threshold of absdiff", thresholdImage);
			cv::blur(thresholdImage, thresholdImage, cv::Size(BLUR_SIZE, BLUR_SIZE));
			cv::threshold(thresholdImage, thresholdImage, SENSITIVITY_VALUE2, 255, THRESH_BINARY);
			//cv::imshow("Threshold after blur", thresholdImage);
			oldCenter = center;
			center.clear();

			searchForMovement(thresholdImage, frame1);



			if (frameCount > 10 && center.at(1) < maxCenter && center.at(1) > 100){
				maxCenter = center.at(1);
				//cout << "\n!!! maxCenter: " << maxCenter;
			}
			if (center.at(1) == 0){
				maxCenter = 1000;
			}

			if ((center.at(1) - maxCenter) > fallHeight){
				didFall = true;
				//cout << "\n!!! FALL: ";
			}
			/*
			if (didFall){
				cout << "COORDINATES-->  X : " << center.at(0) << " WIDTH: " << w << endl;
				cout << "COORDINATES-->  Y : " << center.at(1) << " HEIGHT: " << h << endl;
				cout << "---------------------------\n";
			}
			*/
			if (didFall){
				fallInverval++;
				capturingFallingVideo = true;
			}
			if (fallInverval > 10){
				didFall = false;
				fallInverval = 0;
				videoWriter.release();
				sendMail(eMailAddress, "Lutfen hastanizin durumunu kontrol ediniz!");
				fallCount++;
				videoWriter.open("out"+to_string(fallCount)+".avi", CV_FOURCC('M', 'J', 'P', 'G'), 10, Size(frame_width, frame_height), true);
				//sendMessage(ipAddress, message);
			}

			if (didFall){
				putText(frame1, "PATIENT FELL", Point(80, 80), 1, 2, Scalar(0, 0, 255), 3);
			}

			cv::imshow("Main Window", frame1);
		}
		else{
			cout << "CAN'T READ IMAGE FROM WEBCAM!! \n PRESS ESC TO QUIT";
		}

		switch (waitKey(10)){
		case 27: //EXIT if esc is pressed.
			return 0;
		case 32: //PAUSE if the space bar is pressed
			isPaused = !isPaused;
			if (isPaused == true){
				while (isPaused == true){
					if (waitKey() == 32)
						isPaused = false;
				}
			} break;
		case 83:
		case 115: // S 
			cout << "Enter new Sensivity value of absdiff threshold: (DEFAULT 20) \n";
			cin >> SENSITIVITY_VALUE1;
			cout << "Enter new Sensivity value of blurred threshold: (DEFAULT 27) \n";
			cin >> SENSITIVITY_VALUE2;
			break;
		case 66:
		case 98: // B
			cout << "Enter new Blur value: (DEFAULT 19) \n";
			cin >> BLUR_SIZE;
			break;
		case 70:
		case 102: // F
			cout << "Enter new max motion counter value: (DEFAULT 3) \n";
			cin >> MAX_MOTION_COUNTER;
			break;
		case 69:
		case 101: // E
			cout << "Enter the minimum width for object: (Default 50) \n";
			cin >> minW;
			cout << "Enter the maximum width for object: (Default 450)\n";
			cin >> maxW;
			cout << "Enter the minimum height for object: (Default 180)\n";
			cin >> minH;
			cout << "Enter the maximum height for object: (Default 450)\n";
			cin >> maxH;
			break;
		case 65:
		case 97: // A
			cout << "Enter the new fall interval: (Default 10)\n";
			cin >> fallInverval;
			break;

		case 67:
		case 99: // C
			cout << "Enter the new fall height: (Default 100)\n";
			cin >> fallHeight;
			break;
		case 68:
		case 100: // D
			minW = 50;
			maxW = 450;
			minH = 180;
			maxH = 450;
			SENSITIVITY_VALUE1 = 20;
			SENSITIVITY_VALUE2 = 27;
			fallInverval = 10;
			fallHeight = 100;
			BLUR_SIZE = 19 ;
			MAX_MOTION_COUNTER = 3;
			cout << "Everything is set to default values \n";

			break;

		}// END OF SWITCH
		if (frameCount < 11)
		frameCount++;

	} // MAIN LOOP

	return 0;
}
