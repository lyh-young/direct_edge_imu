/**
* This file is part of the implementation of our papers: 
* [1] Yonggen Ling, Manohar Kuse, and Shaojie Shen, "Direct Edge Alignment-Based Visual-Inertial Fusion for Tracking of Aggressive Motions", Autonomous Robots, 2017.
* [2] Yonggen Ling and Shaojie Shen, "Aggresive Quadrotor Flight Using Dense Visual-Inertial Fusion", in Proc. of the IEEE Intl. Conf. on Robot. and Autom., 2016.
* [3] Yonggen Ling and Shaojie Shen, "Dense Visual-Inertial Odometry for Tracking of Aggressive Motions", in Proc. of the IEEE International Conference on Robotics and Biomimetics 2015.
*
*
* For more information see <https://github.com/ygling2008/direct_edge_imu>
*
* This code is a free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This code is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this code. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ___RGB_ODOMETRY_H___
#define ___RGB_ODOMETRY_H___

// ROS Headers
#include <ros/ros.h>
#include <ros/console.h>

#include <tf/tfMessage.h>
#include <tf/transform_listener.h>

#include<geometry_msgs/PoseStamped.h>
#include<nav_msgs/Path.h>

// Std Headers
#include <fstream>
#include <iostream>
#include <string>
#include <numeric>
#include <vector>

// Linear Algebra
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <Eigen/Sparse>
#include <igl/repmat.h>

// OpenCV Headers
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/eigen.hpp>

// OpenCV-ROS interoperability headers
#include <cv_bridge/cv_bridge.h>
#include <image_transport/image_transport.h>

// RGBD custom message
#include "FColorMap.h"
//#include "MentisVisualHandle.h"
//#include "GOP.h"
#include "types.h"
#include "SophusUtil.h"


#define GRAD_NORM( A, B ) (fabs(A) + fabs(B))
//#define GRAD_NORM( A, B ) fabs(A)

//
// Display Plugs (Each Iteration wait plugs)
//#define __SHOW_REPROJECTIONS_EACH_ITERATION__
//#define __SHOW_REPROJECTIONS_EACH_ITERATION__DISPLAY_ONLY
//#define __PRINT_POSE_EACH_ITERATION 0 //display the data only for this level in runIterations
//#define __PRINT_GRAD_DIRECTION_EACH_ITERATION


#define __REPROJECTION_LEVEL 0 //only useful with above defines



//#define __ENABLE_DISPLAY__  1  //display in loop()
#define __MINIMAL_DISPLAY 1




//
// Ground truth Plugs
#define __TF_GT__ //Enable GT DISPLAY
#define __WRITE_EST_POSE_TO_FILE "poses/estPoses.txt"
#define __WRITE_GT__POSE_TO_FILE "poses/gtPoses.txt"


//
// Updating the reference frame logic plugs
#define __NEW__REF_UPDATE //throwing away wrong estimate
//#define __OLD__REF_UPDATE //old naive logic


//
// Interpolate distance transform plug
//#define __INTERPOLATE_DISTANCE_TRANSFORM


//
// Scale Distance transform plug. ie. scale distance transfrom to 0-255.
// note: If you do not scale you cannot really view the overlay on the DT. Not scaling gives better pose estimates
#define __SCALE_NORMALIZE_DISTANCE_TRANFROM

//
// Attempt rotationization (with SVD) at each iteration
#define __ENABLE_ROTATIONIZE__


//
// Enable regularization term
#define __ENABLE_L2_REGULARIZATION


//
// Read data from files plug.
//      Note: This is not ensuring that dvo does not listen to streams (need to take care of this by yourself)
//      Note: Cannot be used together with __TF_GT__ because the xml files do not contain GT data
//#define __DATA_FROM_XML_FILES__ "TUM_RGBD/fr2_desk"
//#define __DATA_FROM_XML_FILES__START 0 //compulsory
//#define __DATA_FROM_XML_FILES__END 2500 //compulsory
//#define __DATA_SKIP_FACTOR 4 //use 1 for processing every frame. 2 will give every alternate frame, 3 will gv every 3rd frame and so on

#undef NDEBUG
#include <assert.h>



//
// Writing iteration image reprojection files
//#define __WRITE_EACH_ITERATION_IMAGE_TO_FILE



typedef std::vector<Eigen::RowVectorXf> JacobianList;
typedef Eigen::MatrixXf ImCordList;  //2xN
typedef Eigen::MatrixXf SpaceCordList; //3xN
//typedef std::vector<float> IntensityList;
typedef Eigen::VectorXf IntensityList;
typedef Eigen::MatrixXf JacobianLongMatrix;


// inverse formulation of dist-transform
typedef Eigen::Matrix<float, 1, 6> RowVector6f;


/// Defines the class to handle streaming images and compute camera pose from RGBD images ie. dense visual odometry (DVO)
class SolveDVO
{
 //   friend class MentisVisualHandle;

public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    SolveDVO();
    ~SolveDVO();

//    void loopDry();
//    void loop();
//    void loopFromFile();


    // Causal Testing for the icra-2016 paper
    void casualTestFunction();
#ifdef __WRITE_EACH_ITERATION_IMAGE_TO_FILE
    int FRG34h_casualIterationCount;
#endif

    void setCameraMatrix(const char* calibFile);


    // Camera params
    bool isCameraIntrinsicsAvailable;
    cv::Mat cameraMatrix, distCoeffs;
    Eigen::Matrix3f K, K_inv; //same as cameraMatrix
    float fx, fy, cx, cy;

    Eigen::MatrixXf reprojections;
    Eigen::VectorXf epsilon;
    Eigen::VectorXf weights ;

    //
    // Received Frame
    //cv::Mat rcvd_frame, rcvd_dframe; ///< Received RGBD data
    std::vector<Eigen::MatrixXf> rcvd_framemono, rcvd_depth; ///< Received image pyramid
    bool isFrameAvailable;


    //
    // Previously set now frames (added 14th Aug, 2015)
    std::vector<Eigen::MatrixXf> p_now_framemono, p_now_depth; ///< previously set now image pyramid
    bool isPrevFrameAvailable;
//    void setPrevFrameAsRefFrame(); // sets (p_now_framemono, p_now_depth) as ref frame


    //
    // Now & Ref Frames
    std::vector<Eigen::MatrixXf> im_n, dim_n; ///< Now frames
    std::vector<Eigen::MatrixXf> im_r, dim_r; ///< Reference frames
//    void setRcvdFrameAsRefFrame();
//    void setRcvdFrameAsNowFrame();
    bool isNowFrameAvailable, isRefFrameAvailable;



    //
    // Frame pose handling class
    //GOP<double> gop;



    //
    // Functions relating to the Gauss-Newton Iterations
    std::vector<JacobianList> _J;
    std::vector<ImCordList> _imCord;
    std::vector<SpaceCordList> _spCord;
    std::vector<IntensityList> _intensities;
    std::vector<Eigen::MatrixXi> _roi;
    bool isJacobianComputed;
    //void computeJacobian();
    //void computeJacobian(int level, JacobianList &J, ImCordList &imC, SpaceCordList &spC, IntensityList &I, Eigen::MatrixXi &refROI );
    float runIterations(STATE *keyFrame, STATE *currentFrame, CALIBRATION_PAR *cali, int level, int maxIterations, Eigen::Matrix3d &cR, Eigen::Vector3d &cT,
                                Eigen::VectorXf& energyAtEachIteration, Eigen::VectorXf& finalEpsilons,
                                Eigen::MatrixXf& finalReprojections, int& bestEnergyIndex, float & finalVisibleRatio );
    void updateEstimates( Eigen::Matrix3f& cR, Eigen::Vector3f& cT, Eigen::Matrix3f& xRot, Eigen::Vector3f& xTrans );

    float optimizeGaussianNewton(STATE* keyFrame, STATE* currentFrame, CALIBRATION_PAR* cali,
                                 int level, int maxIterations, Eigen::Matrix3d& cR, Eigen::Vector3d& cT,
                                 Eigen::VectorXf& energyAtEachIteration, Eigen::VectorXf& finalEpsilons,
                                 Eigen::MatrixXf& finalReprojections, int& bestEnergyIndex, float& finalVisibleRatio);

    float getWeightOf( float r );
    //int selectedPts(int level, Eigen::MatrixXi &roi);
    void rotationize( Eigen::Matrix3d &R);
    float interpolate( Eigen::MatrixXf &F, float ry, float rx );
    float aggregateEpsilons();


    bool signalGetNewRefImage;
    char signalGetNewRefImageMsg[500];


    // helpers
    void imageGradient( Eigen::MatrixXf &image, Eigen::MatrixXf& gradientX, Eigen::MatrixXf &gradientY );
    void to_se_3(Eigen::Vector3f& w, Eigen::Matrix3f& wx);
    void to_se_3(float w0, float w1, float w2, Eigen::Matrix3f& wx);
    void exponentialMap(Eigen::VectorXf &psi, Eigen::Matrix3f &outR, Eigen::Vector3f &outT); //this is not in use. It is done with Sophus
    void sOverlay( Eigen::MatrixXf eim, Eigen::MatrixXi mask, cv::Mat &outIm, cv::Vec3b color);
    void printRT( Eigen::Matrix3f &fR, Eigen::Vector3f &fT, const char *msg );
    void printRT( Eigen::Matrix3d &fR, Eigen::Vector3d &fT, const char *msg );
    void printPose( geometry_msgs::Pose& rospose, const char * msg, std::ostream &stream );
    void printDescentDirection( Eigen::VectorXd direction, const char * msg );
    void printDescentDirection( Eigen::VectorXf direction, const char * msg );
    float getDriftFromPose( geometry_msgs::Pose& p1, geometry_msgs::Pose& p2 );
    void  analyzeDriftVector( std::vector<float>& v );
    float processResidueHistogram( Eigen::VectorXf &residi, bool quite );
    void visualizeEnergyProgress(Eigen::VectorXf energyAtEachIteration, int bestEnergyIndex, int XSPACING ) ;
    //void visualizeResidueHeatMap( Eigen::MatrixXf& eim, Eigen::MatrixXf& residueAt );
    void visualizeDistanceResidueHeatMap(cv::Mat& tmpIm, Eigen::MatrixXi &reprojectionMask, Eigen::MatrixXf &nowDistMap ) ;

//    void visualizeReprojectedDepth( Eigen::MatrixXf& eim, Eigen::MatrixXf& reprojDepth );


    // debugging variable
    Eigen::MatrixXi __now_roi_reproj; //this is a `binary` mask of reprojected points
    Eigen::VectorXf __residues;       //list of residue values, -1 ==> this pixel is not visible in reprojected frame
    Eigen::MatrixXf __now_roi_reproj_values; //this is a mask with float values (= residues at that point)
    Eigen::MatrixXf __reprojected_depth; //mask of float values denoting depth values (in mm) in now frames
    Eigen::MatrixXf __weights;


    // distance transform related
//    bool isRefDistTransfrmAvailable;
//    std::vector<Eigen::MatrixXf> ref_distance_transform;
//    std::vector<Eigen::MatrixXi> ref_edge_map;
//    std::vector<Eigen::MatrixXf> ref_DT_gradientX;
//    std::vector<Eigen::MatrixXf> ref_DT_gradientY;
    //void computeDistTransfrmOfRef();

//    bool isNowDistTransfrmAvailable;
//    std::vector<Eigen::MatrixXf> now_distance_transform;
//    std::vector<Eigen::MatrixXi> now_edge_map;
//    std::vector<Eigen::MatrixXf> now_DT_gradientX;
//    std::vector<Eigen::MatrixXf> now_DT_gradientY;
    //void computeDistTransfrmOfNow();

//    void visualizeDistanceResidueHeatMap(Eigen::MatrixXf& eim, Eigen::MatrixXi& reprojectionMask, Eigen::MatrixXf& nowDistMap );
//    void visualizeEnergyProgress( Eigen::VectorXf energyAtEachIteration, int bestEnergyIndex, int XSPACING=1 );





    //
    //other debuging functions
    void imshowEigenImage(const char *winName, Eigen::MatrixXd& eim);
    void imshowEigenImage(const char *winName, Eigen::MatrixXf& eim);
    bool loadFromFile( const char * xmlFileName );
    std::string cvMatType2str(int type);



    //
    // Forward formulation (distance energy function) related function (8th July, 2015)
    // selectedPts //< already declared above
    std::vector<SpaceCordList> _ref_edge_3d; ///< Stores the edge points of the reference frame in 3d ie. `E_i`
    std::vector<ImCordList> _ref_edge_2d; ///< Stores edge image co-ordinates (2d) ie. `e_i`
    std::vector<Eigen::MatrixXi> _ref_roi_mask; ///< Reference selected edge points


    // void gaussNewton <-- defined above
    void enlistRefEdgePts( int level, Eigen::MatrixXi &refEdgePtsMask, SpaceCordList& _3d, ImCordList& _2d );
    //void preProcessRefFrame();
    void computeJacobianOfNowFrame(STATE *keyFrame, STATE *currentFrame, CALIBRATION_PAR *cali, int level, Eigen::Matrix3f& cR, Eigen::Vector3f& cT, JacobianLongMatrix& Jcbian );
    float getReprojectedEpsilons(STATE* keyFrame, STATE* currentFrame, CALIBRATION_PAR* cali, int level);

    void cordList_2_mask( Eigen::MatrixXf& list, Eigen::MatrixXi &mask); //make sure mask is preallocated



    //
    // Publishers & Subscribers
    ros::NodeHandle nh;
    ros::Subscriber sub;

    //
    // Visualization Class
    //MentisVisualHandle mviz;



    //helpers for publishing
    void tfTransform2EigenMat( tf::StampedTransform& tr, Eigen::Matrix3f& R, Eigen::Vector3f& T );


//    // Poses to File
//#ifdef __WRITE_EST_POSE_TO_FILE
//    std::ofstream estPoseFile;
//#endif



//#ifdef __WRITE_GT__POSE_TO_FILE
//    std::ofstream gtPoseFile;
//#endif



    // constants
    float grad_thresh;
    float ratio_of_visible_pts_thresh; //should be between [0, 1]
    float laplacianThreshExitCond; //set this to typically 15-20
    float psiNormTerminationThreshold; //terminate iteration if norm of psi falls below this value
    float trustRegionHyperSphereRadius; //trust region to scale the direction of descent

    std::vector<int> iterationsConfig;
};





#endif //___RGB_ODOMETRY_H___
