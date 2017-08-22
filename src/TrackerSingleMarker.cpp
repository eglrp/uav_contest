/**
 * Copyright (C) 2010  ARToolkitPlus Authors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *  Daniel Wagner
 *  Pavel Rojtberg
 */

#include <iostream>
#include <ARToolKitPlus/TrackerSingleMarker.h>

using std::cerr;
using std::endl;

namespace ARToolKitPlus {

TrackerSingleMarker::TrackerSingleMarker(int imWidth, int imHeight, int maxImagePatterns, int pattWidth, int pattHeight,
        int pattSamples, int maxLoadPatterns) :
    Tracker(imWidth, imHeight, maxImagePatterns, pattWidth, pattHeight, pattSamples, maxLoadPatterns) {
    thresh = 100;
    patt_width = 2.0;
    patt_center[0] = patt_center[1] = 0.0;
}
bool TrackerSingleMarker::init(const char* nCamParamFile, ARFloat nNearClip, ARFloat nFarClip) {
    if (!this->checkPixelFormat()) {
        cerr << "ARToolKitPlus: Invalid Pixel Format!" << endl;
        return false;
    }

    // initialize applications
    if (nCamParamFile) {
        return loadCameraFile(nCamParamFile, nNearClip, nFarClip);
    }

    return true;
}

bool TrackerSingleMarker::initWithoutCameraFile()
{
    if (!this->checkPixelFormat()) {
        cerr << "ARToolKitPlus: Invalid Pixel Format!" << endl;
        return false;
    }
    
    Camera* cam = new Camera();
    cam->xsize = 400;
    cam->ysize = 400;
    cam->cc[0] = cam->cc[1] = 200;
    cam->fc[0] = cam->fc[1] = 150;
    
    std::fill_n(cam->kc,6,0);
    cam->undist_iterations = 0;
    
    cam->mat[0][0] = cam->fc[0]; // fc_x
    cam->mat[1][1] = cam->fc[1]; // fc_y
    cam->mat[0][2] = cam->cc[0]; // cc_x
    cam->mat[1][2] = cam->cc[1]; // cc_y
    cam->mat[2][2] = 1.0;
    
    if (arCamera)
        delete arCamera;

    arCamera = NULL;
    setCamera(cam, 1.0f, 1000.0f);
    return true;
}

std::vector<int> TrackerSingleMarker::calc(const uint8_t* nImage, ARMarkerInfo** nMarker_info, int* nNumMarkers) {
    std::vector<int> detected;

    if (nImage == NULL)
        return detected;

    confidence = 0.0f;

    // detect the markers in the video frame
    //
    if (arDetectMarker(nImage, this->thresh, &marker_info, &marker_num) < 0) {
        return detected;
    }

    // copy all valid ids
    for (int j = 0; j < marker_num; j++) {
        if (marker_info[j].id != -1) {
            detected.push_back(marker_info[j].id);
        }
    }

    if (nMarker_info)
        *nMarker_info = marker_info;

    if (nNumMarkers)
        *nNumMarkers = marker_num;

    return detected;
}

int TrackerSingleMarker::selectBestMarkerByCf() {
    // find best visible marker
    int best = -1;

    for (int j = 0; j < marker_num; j++) {
        if (marker_info[j].id != -1) {
            if (best == -1)
                best = j;
            else if (marker_info[best].cf < marker_info[j].cf)
                best = j;
        }
    }

    if (best != -1) {
        // there was something detected
        best = marker_info[best].id; // we want the id and not the offset
        selectDetectedMarker(best);
    }

    return best;
}

void TrackerSingleMarker::selectDetectedMarker(const int id) {
    for (int i = 0; i < marker_num; i++) {
        if (marker_info[i].id == id) {
            executeSingleMarkerPoseEstimator(&marker_info[i], patt_center, patt_width, patt_trans);
            convertTransformationMatrixToOpenGLStyle(patt_trans, this->gl_para);
            confidence = marker_info[i].cf;
        }
    }
}

int TrackerSingleMarker::addPattern(const char* nFileName) {
    int patt_id = arLoadPatt(const_cast<char*> (nFileName));
    if (patt_id < 0) {
        cerr << "ARToolKitPlus: error loading pattern " << nFileName << endl;
    }
    return patt_id;
}

void TrackerSingleMarker::getARMatrix(ARFloat nMatrix[3][4]) const {
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 4; j++)
            nMatrix[i][j] = patt_trans[i][j];
}

const ARMarkerInfo* TrackerSingleMarker::getMarkerInfoById(const int id) const
{
    for (int i = 0; i < marker_num; i++) {
        if (marker_info[i].id == id) {
            return &marker_info[i];
        }
    }
    return 0;
}

} // namespace ARToolKitPlus
