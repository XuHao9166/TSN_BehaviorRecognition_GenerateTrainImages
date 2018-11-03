#include <windows.h> // for windows systems
#include <algorithm>    // std::sort
#include <stdio.h>
#include <iostream>
#include <string>


#include < opencv2\opencv.hpp>
#include < opencv2/core/core.hpp>
#include < opencv2/highgui/highgui.hpp>
#include < opencv2/video/background_segm.hpp>
//#include < opencv2\gpu\gpu.hpp>


#include <cuda_runtime.h>  
#include <cuda.h> 

#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudafilters.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <opencv2/cudabgsegm.hpp> 
#include <opencv2/cudalegacy.hpp>

#include <opencv2/cudacodec.hpp>
#include <opencv2/cudafeatures2d.hpp>
#include <opencv2/cudaobjdetect.hpp>
#include <opencv2/cudaoptflow.hpp> 
#include <opencv2/cudastereo.hpp>
#include <opencv2/cudawarping.hpp> 





#define WINDOWS 1
using namespace cv;
using namespace std;

#define UNKNOWN_FLOW_THRESH 1e9  





 void convertFlowToImage(const Mat &flow_x, const Mat &flow_y, Mat &img_x, Mat &img_y,
	 double lowerBound, double higherBound)
 {
#define CAST(v, L, H) ((v) > (H) ? 255 : (v) < (L) ? 0 : cvRound(255*((v) - (L))/((H)-(L))))
	 for (int i = 0; i < flow_x.rows; ++i) {
		 for (int j = 0; j < flow_y.cols; ++j) {
			 float x = flow_x.at<float>(i, j);
			 float y = flow_y.at<float>(i, j);
			 img_x.at<uchar>(i, j) = CAST(x, lowerBound, higherBound);
			 img_y.at<uchar>(i, j) = CAST(y, lowerBound, higherBound);
		 }
	 }
#undef CAST
 }


 int readFilenames(std::vector<string> &filenames, const string &directory)  ///读取文件夹中文件的数量
 {
#ifdef WINDOWS
	 HANDLE dir;
	 WIN32_FIND_DATA file_data;

	 if ((dir = FindFirstFile((directory + "/*").c_str(), &file_data)) == INVALID_HANDLE_VALUE)
		 return 0; /* No files found */

	 do {
		 const string file_name = file_data.cFileName;
		 const string full_file_name = directory + "/" + file_name;
		 const bool is_directory = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

		 if (file_name[0] == '.')
			 continue;

		 if (is_directory)
			 continue;

		 filenames.push_back(full_file_name);
	 } while (FindNextFile(dir, &file_data));

	 FindClose(dir);
#else
	 DIR *dir;
	 class dirent *ent;
	 class stat st;

	 dir = opendir(directory.c_str());
	 while ((ent = readdir(dir)) != NULL) {
		 const string file_name = ent->d_name;
		 const string full_file_name = directory + "/" + file_name;

		 if (file_name[0] == '.')
			 continue;

		 if (stat(full_file_name.c_str(), &st) == -1)
			 continue;

		 const bool is_directory = (st.st_mode & S_IFDIR) != 0;

		 if (is_directory)
			 continue;

		 //        filenames.push_back(full_file_name); // returns full path
		 filenames.push_back(file_name); // returns just filename
	 }
	 closedir(dir);
#endif
	 std::sort(filenames.begin(), filenames.end()); //optional, sort the filenames
	 return(filenames.size()); //返回查找到的文件数
 } // 在路径下获取文件


 



 int main()
{
		
	static Ptr<cuda::OpticalFlowDual_TVL1> fbOF = cuda::OpticalFlowDual_TVL1::create();
	
	////需要读取的批量文件所在的文件夹
	string folder = "./inputVideo";  ////输入全部视频所在的路径，截止至他们所在的文件夹

	cout << "正在读取文件夹： " << folder << endl;
	vector<string> filenames;

	int num_files = readFilenames(filenames, folder);
	cout << "总的文件数量 = " << num_files << endl;  ///显示文件夹中文件的数量

	for (size_t i = 0; i < filenames.size(); ++i)  ////批量读取一个文件夹中的全部视频
	{

		int s = 1;
		Mat GetImg, next, prvs;
		unsigned long AAtime = 0, BBtime = 0;
		double Bound = 20;
		//gpu 变量
		cv::cuda::GpuMat prvs_gpu, next_gpu, flow_gpu;
		cv::cuda::GpuMat prvs_gpu_o, next_gpu_o;
		cv::cuda::GpuMat prvs_gpu_c, next_gpu_c;
		static cv::cuda::GpuMat discrete[2];


		cout << filenames[i] << endl;


		string a = filenames[i]; ///#include <string>
		string videoname = a.substr(a.length() - 21, 17);  ///截取字符串a 的第a.length() - 18号字符之后的14个字符
		cout << videoname << endl;

		///创建文件夹  
		string image_input_path = "../endImage/";  ///#include <direct.h>
		image_input_path += videoname;
		bool flag = CreateDirectory(image_input_path.c_str(), NULL);  ///创建成功
		cout << "创建文件夹：" << videoname << "成功！" << endl;


		VideoCapture stream1(filenames[i]);
	
	   if (!(stream1.read(GetImg))) 
		  return 0;


	resize(GetImg, GetImg, Size(340, 256));
	//////////////////////////////////////////////////////////////////////////////////////////////
	
	
	prvs_gpu_o.upload(GetImg);
	cv::cuda::resize(prvs_gpu_o, prvs_gpu_c, Size(GetImg.size().width / s, GetImg.size().height / s));
	cv::cuda::cvtColor(prvs_gpu_c, prvs_gpu, CV_BGR2GRAY);

	/////////////////////////////////////////////////////////////////////////////////////////////

	//稠密光流
	//cv::cuda::FarnebackOpticalFlow fbOF;
	//static Ptr<cuda::FarnebackOpticalFlow> fbOF = cuda::FarnebackOpticalFlow::create();
	int framenum = 0;
	char file_name[100];
	char file_x_name[100];
	char file_y_name[100];
	while (true) {

		framenum++;

		string X  = image_input_path + "\\"+ "flow_x_%d.jpg";
		const char* X_path = X.c_str();

		string Y = image_input_path + "\\" + "flow_y_%d.jpg";
		const char* Y_path = Y.c_str();

		string S = image_input_path + "\\" + "img_%d.jpg";
		const char* S_path = S.c_str();

		sprintf(file_x_name, X_path , framenum);
		sprintf(file_y_name, Y_path , framenum);
		sprintf(file_name, S_path, framenum);

		/*cout << file_x_name << endl;
		cout << file_y_name << endl;
		cout << file_name << endl;*/
		if (!(stream1.read(GetImg))) //get one frame form video   
			break;

		resize(GetImg, GetImg, Size(340, 256));
		imshow("src", GetImg);
		///////////////////////////////////////////////////////////////////
		//将变量写入GPU
		next_gpu_o.upload(GetImg);
		cv::cuda::resize(next_gpu_o, next_gpu_c, Size(GetImg.size().width / s, GetImg.size().height / s));
		cv::cuda::cvtColor(next_gpu_c, next_gpu, CV_BGR2GRAY);
		///////////////////////////////////////////////////////////////////

		AAtime = getTickCount();
		//稠密光流计算
		fbOF->calc(prvs_gpu, next_gpu, flow_gpu);
		//fbOF(prvs_gpu, next_gpu, flow_x_gpu, flow_y_gpu);
		BBtime = getTickCount();
		float pt = (BBtime - AAtime) / getTickFrequency();
		float fpt = 1 / pt;
		//printf("%.2lf / %.2lf \n", pt, fpt);


		/////将计算得到的整体光流分解成x,y两个方向的光流	
		cuda::split(flow_gpu, discrete);

		Mat flow_x(discrete[0]); ////对函数输出的双通道光流图像进行分解
		Mat flow_y(discrete[1]);
		//////////////

		/////////////归一化光流可视化
		Mat img_x(flow_x.size(), CV_8UC1); ///输出必须定义为单通道
		Mat img_y(flow_y.size(), CV_8UC1);
		convertFlowToImage(flow_x, flow_y, img_x, img_y, -Bound, Bound);

		imshow("img_x", img_x);
		imshow("img_y", img_y);


		imwrite(file_x_name, img_x);
		imwrite(file_y_name, img_y);
		imwrite(file_name, GetImg);
		///////////////////////////////////////////////////////////////////

		prvs_gpu = next_gpu.clone();

		if (waitKey(5) >= 0)
			break;
	 }
	}
}



