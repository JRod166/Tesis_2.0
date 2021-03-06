#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "pattern_point.h"
#include "../calibration_utils.h"
#include <algorithm>

using namespace cv;
using namespace std;

#define flag_masa 0

void order_points(Mat &out, vector<PatternPoint> &new_pattern_points,int pattern_cols,int pattern_rows);
void update_mask_from_points(vector<PatternPoint> points, int w, int h, Point mask_point[][4]);
int mode_from_father(vector<PatternPoint> pattern_points);
float angle_between_two_points(PatternPoint p1, PatternPoint p2);
float distance_to_rect(PatternPoint p1, PatternPoint p2, PatternPoint x);
vector<PatternPoint> more_distant_points(vector<PatternPoint>points);
float avgColinearDistance(vector<PatternPoint> &points);
float avgColinearDistance(vector<Point2f> &points);
float avgColinearDistance_old(vector<Point2f> &points);

/**
 * @details Function to support PatternPoint sort by hierarchy
 *
 * @param p1 First point
 * @param p2 Second point
 *
 * @return father hierarchy of point 1 is lower than father hierarchy of point 2
 */
bool sort_pattern_point_by_father(PatternPoint p1, PatternPoint p2) {
    return p1.h_father < p2.h_father;
}
bool sort_pattern_point_by_x(PatternPoint p1, PatternPoint p2) {
    return p1.x < p2.x;
}
bool sort_pattern_point_by_y(PatternPoint p1, PatternPoint p2) {
    return p1.y < p2.y;
}
/**
 * @details Return the father hierarchy mode from a vector of PatternPoints
 *
 * @param pattern_points Vector of points
 * @return father hierarchy mode
 */
int mode_from_father(vector<PatternPoint> points) {
    if (points.size() == 0) {
        return -1;
    }
    vector<PatternPoint> temp;
    for (int p = 0; p < points.size(); p++) {
        temp.push_back(points[p]);
    }
    sort(temp.begin(), temp.end(), sort_pattern_point_by_father);

    int number = temp[0].h_father;
    int mode = number;
    int count = 1;
    int countMode = 1;

    for (int p = 1; p < temp.size(); p++) {
        if (number == temp[p].h_father) {
            count++;
        } else {
            if (count > countMode) {
                countMode = count;
                mode = number;
            }
            count = 1;
            number =  temp[p].h_father;
        }
    }
    if (count > countMode) {
        return number;
    } else {
        return mode;
    }
}

int find_pattern_points(Mat original, int w, int h, vector<Point2f> &pattern_points, int pattern_cols, int pattern_rows, bool showWindows = false) {
    int total_points = pattern_rows * pattern_cols;
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    vector<PatternPoint> ellipses_temp;
    vector<PatternPoint> new_pattern_points;
    float radio_hijo;
    float radio;
    float radio_prom = 0;
    float distance;
    Scalar purple(255, 0, 255);
    Scalar red(0, 0, 255);
    Scalar yellow(0, 255, 255);
    Scalar blue(255, 0, 0);
    Scalar green(0, 255, 0);
    Scalar white(255, 255, 255);

    Mat src_gray, masked, thresh;
    masked = original.clone();
    cvtColor( original, src_gray, cv::COLOR_BGR2GRAY );
    adaptiveThreshold(src_gray, thresh, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 41, 12);
    segmentar(src_gray, src_gray, thresh, w, h);
	if (showWindows) {
		imshow("Threshold",src_gray);
	}

    int erosion_size = 2;
    Mat kernel = getStructuringElement( MORPH_ELLIPSE,
                                        Size( 2 * erosion_size + 1, 2 * erosion_size + 1 ),
                                        Point( erosion_size, erosion_size ) );

    
    findContours( src_gray, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, Point(0, 0) );
    /* Find ellipses with a father and a son*/
    for (int c = 0; c < contours.size(); c++) {
        if (contours[c].size() > 4 && hierarchy[c][3] != -1) {
            RotatedRect elipse  = fitEllipse( Mat(contours[c]) );
            radio = (elipse.size.height + elipse.size.width) / 4;
            if (hierarchy[c][2] != -1) { //If has a son
                int hijo  = hierarchy[c][2];
                if (contours[hijo].size() > 4 ) {
                    RotatedRect elipseHijo  = fitEllipse( Mat(contours[hijo]) );
                    radio_hijo = (elipseHijo.size.height + elipseHijo.size.width) / 4;
                    /* Check center proximity */
                    if ( /*radio <= radio_hijo * 2 &&*/ cv::norm(elipse.center - elipseHijo.center) < radio_hijo / 2) {
                        if (flag_masa) {
                            auto mu_son = moments(contours[hijo]);
                            auto mu_dad = moments(contours[c]);
                            float x = (mu_son.m10 / mu_son.m00 + mu_dad.m10 / mu_dad.m00) / 2.0;
                            float y = (mu_son.m01 / mu_son.m00 + mu_dad.m01 / mu_dad.m00) / 2.0;
                            ellipses_temp.push_back(PatternPoint(x, y, radio, hierarchy[c][3]));
                        }
                        else {
                            ellipses_temp.push_back(PatternPoint((elipse.center.x + elipseHijo.center.x ) / 2,
                                                                 (elipse.center.y + elipseHijo.center.y ) / 2,
                                                                 radio,
                                                                 hierarchy[c][3]));
                            /*ellipses_temp.push_back(PatternPoint(elipse.center.x,
                                                                 elipse.center.y,
                                                                radio,
                                                                hierarchy[c][3]));*/
                            /*ellipses_temp.push_back(PatternPoint(elipseHijo.center.x,
                                                                 elipseHijo.center.y,
                                                                radio,
                                                                hierarchy[c][3]));*/
                        }
                        try
                        {
                            ellipse(masked, elipseHijo, red, 2);
                            //ellipse(src_gray, elipseHijo, red, 2);
                            ellipse(masked, elipse, yellow, 2);
                            //ellipse(src_gray, elipse, yellow, 2);
                        }
                        catch(const std::exception& e)
                        {
//                            cout<<"elipse fuera de rango"<<endl;
                        }
                        
                    }
                }
            } else {
                try
                {
                    ellipse(masked, elipse, purple, 2);
                    //ellipse(src_gray, elipse, purple, 2);
                }
                catch(const std::exception& e)
                {
                    //cout<<"elipse fuera de rango"<<endl;
                }
                
            }
        }
        drawContours( src_gray, contours, c, white, 2, 8, hierarchy, 0, Point() );
    }
	if (showWindows)
	{
		imshow("Contours",src_gray);
	}
    /* Filter ellipses who doesn't have another ones near to it */
    int count;
    for (int i = 0; i < ellipses_temp.size(); ++i) {
        count = 0;
        for (int j = 0; j < ellipses_temp.size(); ++j) {
            if (i == j) continue;
            radio = ellipses_temp[j].radio;

            distance = ellipses_temp[i].distance(ellipses_temp[j]);
            if (distance < radio * 5/*3.5*/) {
                line(masked, ellipses_temp[i].center(), ellipses_temp[j].center(), red, 1);
                count++;
            }
        }
        if (count >= 2) {
            radio = ellipses_temp[i].radio;
            radio_prom += radio;
            new_pattern_points.push_back(ellipses_temp[i]);
            circle(masked, ellipses_temp[i].center(), radio, blue, 2);
            circle(masked, ellipses_temp[i].center(), 3, yellow, -1);
        }
    }
    radio_prom /= new_pattern_points.size();

    /* Clean false positive checking the father hierarchy */
    if (new_pattern_points.size() > total_points) {
        int mode = mode_from_father(new_pattern_points);
        if (mode != -1 && contours[mode].size() > 4) {
            RotatedRect elipse  = fitEllipse( Mat(contours[mode]) );
            ellipse(masked, elipse, white, 5);

            /* CLEAN USING MODE */
            vector<PatternPoint> temp ;
            for (int e = 0; e < new_pattern_points.size(); e++) {
                if (new_pattern_points[e].h_father == mode) {
                    temp.push_back(new_pattern_points[e]);
                    circle(masked, new_pattern_points[e].center(), new_pattern_points[e].radio, green, 5);
                }
            }
            new_pattern_points = temp;
        }
    }
    if (new_pattern_points.size() == total_points) {
        order_points(original, new_pattern_points,pattern_cols,pattern_rows);
    }
    for (int e = 0; e < new_pattern_points.size(); e++) {
        circle(masked, new_pattern_points[e].center(), new_pattern_points[e].radio, green, 5);
        pattern_points.push_back(new_pattern_points[e].to_point2f());
    }
	if (showWindows) {
		imshow("Ellipses",masked);
		imshow("Line Representation",original);
	}
    
    //update_mask_from_points(new_pattern_points, w, h);
    return new_pattern_points.size();
}

/**
 * @details Build a line using points p1 and p2 and return the distance of this line to the points x
 *
 * @param p1 Point 1 to build the line
 * @param p2 Point 2 to build the line
 * @param x  Point to calc the distance to the line
 * @return Distance from the built line to x point
 */
float distance_to_rect(PatternPoint p1, PatternPoint p2, PatternPoint x) {
    return distance_to_rect(p1.to_point2f(), p2.to_point2f(), x.to_point2f());
}
/**
 * @details Calc the most distant points in a vector of points
 *
 * @param points Poinst to be evaluate
 * @return Most distant points order by x coordinate
 */
vector<PatternPoint> more_distant_points(vector<PatternPoint> points) {
    float distance = 0;
    double temp;
    int p1, p2;
    for (int i = 0; i < points.size(); i++) {
        for (int j = 0; j < points.size(); j++) {
            if (i != j) {
                temp = points[i].distance(points[j]);
                if (distance < temp) {
                    distance = temp;
                    p1 = i;
                    p2 = j;
                }

            }
        }
    }
    if (points[p1].x < points[p2].x) {
        distance = p1;
        p1 = p2;
        p2 = distance;
    }
    vector<PatternPoint> p;
    p.push_back(points[p1]);
    p.push_back(points[p2]);
    return p;
}
/**
 * @details Draw lines patter in drawing Mat from a vector of points
 *
 * @param drawing Mat to draw patter
 * @param pattern_centers Patter points found
 */
void order_points(Mat &drawing, vector<PatternPoint> &new_pattern_points, int pattern_cols, int pattern_rows) {
    int total_points = pattern_cols * pattern_rows;
    if (new_pattern_points.size() < total_points) {
        return;
    }
    vector<PatternPoint> pattern_centers(total_points);
    vector<Scalar> color_palette(5);
    color_palette[0] = Scalar(255, 0, 255);
    color_palette[1] = Scalar(255, 0, 0);
    color_palette[2] = Scalar(0, 255, 0);
    color_palette[3] = Scalar(0, 0 , 255);
    color_palette[4] = Scalar(255, 255 , 0);

    int coincidendes = 0;
    int centers = new_pattern_points.size();
    float pattern_range = 2;
    float distance;
    float min_distance;
    int replace_point;
    int line_color = 0;
    vector<PatternPoint> temp;
    vector<PatternPoint> line_points;
    vector<PatternPoint> limit_points;
    int rows = 0;
    int point = 0;
    centers = new_pattern_points.size();
    for (int i = 0; i < centers; i++) {
        for (int j = 0; j < centers; j++) {
            if (i != j) {
                temp.clear();
                line_points.clear();
                coincidendes = 0;
                for (int k = 0; k < centers; k++) {
                    min_distance = distance_to_rect(new_pattern_points[i], new_pattern_points[j], new_pattern_points[k]);
                    if (min_distance < pattern_range) {
                        coincidendes++;
                        line_points.push_back(new_pattern_points[k]);
                    }
                }

                if (coincidendes == pattern_cols) {
                    sort(line_points.begin(), line_points.end(), sort_pattern_point_by_x);
                    if (line_points[pattern_cols-1].x - line_points[0].x < line_points[0].radio) {
                        sort(line_points.begin(), line_points.end(), sort_pattern_point_by_y);
                    }
                    bool found = false;
                    for (int l = 0; l < limit_points.size(); l++) {
                        if (limit_points[l].x == line_points[0].x && limit_points[l].y == line_points[0].y) {
                            found = true;
                        }
                    }
                    if (!found) {
                        rows++;
                        for (int l = 0; l < line_points.size(); l++) {
                            //pattern_centers.push_back(line_points[l]);
                            pattern_centers[point] = line_points[l];
                            //putText(drawing, to_string(pattern_centers.size() - 1), line_points[l].to_point2f()/*cvPoint(10, 30)*/, FONT_HERSHEY_COMPLEX_SMALL, 1, Scalar(0, 0, 255), 2);
                            putText(drawing, to_string(point), line_points[l].to_point2f()/*cvPoint(10, 30)*/, FONT_HERSHEY_COMPLEX_SMALL, 1, Scalar(0, 0, 255), 2);
                            point++;
                        }
                        limit_points.push_back(line_points[0]);
                        limit_points.push_back(line_points[pattern_cols-1]);
                    }
                }
            }
        }
    }
    
    bool valid_order=true;
    if (rows != pattern_rows) {
        valid_order = false;
        pattern_centers.clear();
    }
    else {
        if(abs(pattern_centers[0].x - pattern_centers[pattern_cols-1].x) < 5){
            valid_order = false;
            pattern_centers.clear();
        } 
    }


    if (pattern_centers.size() == total_points) {
        for(int p=0;p< total_points;p++){
            new_pattern_points[p] = pattern_centers[p];
        }
        //avgColinearDistance(pattern_centers);
        int r;
        for(r = 0; r < pattern_rows-1;r++){
            line(drawing, pattern_centers[r*pattern_cols].to_point2f() , pattern_centers[(r+1)*pattern_cols-1].to_point2f() , color_palette[r], 1);
            line(drawing, pattern_centers[(r+1)*pattern_cols-1].to_point2f() , pattern_centers[(r+1)*pattern_cols].to_point2f() , color_palette[r], 1);
        }
        line(drawing, pattern_centers[r*pattern_cols].to_point2f() , pattern_centers[(r+1)*pattern_cols-1].to_point2f() , color_palette[r], 1);
    }
    else{
        new_pattern_points.clear();
    }

}

float avgColinearDistance(vector<PatternPoint> &points) {
    vector<Point2f> temp(20);
    for (int p = 0; p < 20; p++) {
        temp[p] = points[p].to_point2f();
    }

    //cout << "Prom value " << prom_distance / 12.0 << endl;
    return avgColinearDistance(temp);
}
float avgColinearDistance(vector<Point2f> &points) {
    float prom_distance = 0.0;
    prom_distance += distance_to_rect(points[0], points[4], points[1]);
    prom_distance += distance_to_rect(points[0], points[4], points[2]);
    prom_distance += distance_to_rect(points[0], points[4], points[3]);

    prom_distance += distance_to_rect(points[5], points[9], points[6]);
    prom_distance += distance_to_rect(points[5], points[9], points[7]);
    prom_distance += distance_to_rect(points[5], points[9], points[8]);

    prom_distance += distance_to_rect(points[10], points[14], points[11]);
    prom_distance += distance_to_rect(points[10], points[14], points[12]);
    prom_distance += distance_to_rect(points[10], points[14], points[13]);

    prom_distance += distance_to_rect(points[15], points[19], points[16]);
    prom_distance += distance_to_rect(points[15], points[19], points[17]);
    prom_distance += distance_to_rect(points[15], points[19], points[18]);

    return prom_distance / 12.0 ;
}
float avgColinearDistance_new(vector<Point2f> &pattern_points) {
    float prom_distance = 0.0;
    Vec4f line;
    vector<Point2f> points(5);

    for (int row = 0; row < 4; row++) {
        for (int c = 0; c < 5; c++) {
            points[c] = points[row * 5 + c];
        }
        fitLine(points, line, cv::DIST_L2,  0, 0.01, 0.01);
        Point2f p1;
        Point2f p2;
        p1.x = line[2];
        p1.y = line[3];

        p2.x = p1.x + pattern_points[row * 5 + 4].x * line[0];
        p2.y = p1.y + pattern_points[row * 5 + 4].x * line[1];

        for (int c = 0; c < 5; c++) {
            prom_distance += distance_to_rect(p1, p2, pattern_points[row * 5 + c]);
        }
    }
    //cout << "Acumulated Distance " << prom_distance << endl;
    //cout << "Prom " << prom_distance / 20.0 << endl;
    return prom_distance / 20.0;
}
float avgColinearDistance(vector<vector<Point2f>> points) {
    float f_avg = 0.0;
    for (int s = 0; s < points.size(); s++) {
        f_avg += avgColinearDistance(points[s]);
    }
    return f_avg / points.size();
}