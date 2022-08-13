/**********************************************************************

    Copyright (C) 2012 David Marten <david.marten@qblade.org>

    This program is licensed under the Academic Public License
    (APL) v1.0; You can use, redistribute and/or modify it in
    non-commercial academic environments under the terms of the
    APL as published by the QBlade project; See the file 'LICENSE'
    for details; Commercial use requires a commercial license
    (contact info@qblade.org).

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

***********************************************************************/

#include "WindField.h"

#include <cmath>
#include <ctime>  // For time()
#include <cstdlib>  // For srand() and rand()
#include <limits>
#include <omp.h>
#include <GL/gl.h>
#include <QDebug>
#include <QDate>

#include "../Serializer.h"
#include "../GlobalFunctions.h"
#include "../Params.h"
#include "../Store.h"
#include "../MainFrame.h"
#include "../ParameterViewer.h"
#include "../Graph/NewCurve.h"
#include "../ColorManager.h"
#include "WindFieldModule.h"
#include "../ImportExport.h"


WindField::WindField ()
    : StorableObject ("< no name >",NULL), ShowAsGraphInterface (true){

    m_minValueX = std::numeric_limits<float>::max();
    m_maxValueX = std::numeric_limits<float>::lowest();
    m_minValueY = std::numeric_limits<float>::max();
    m_maxValueY = std::numeric_limits<float>::lowest();
    m_minValueZ = std::numeric_limits<float>::max();
    m_maxValueZ = std::numeric_limits<float>::lowest();

    m_pen.setColor(g_colorManager.getLeastUsedColor(&g_windFieldStore));

    m_shownTimestep = 0;
}

WindField::WindField(ParameterViewer<Parameter::Windfield> *viewer, bool *cancelCalculation)
    : StorableObject ("< no name >",NULL), ShowAsGraphInterface (true){
    viewer->storeObject(this);
	
    m_pen.setColor(g_colorManager.getLeastUsedColor(&g_windFieldStore));

	m_glListIndex = glGenLists(1);
	m_shownTimestep = 0;  // allways shows first timestep first
    m_minValueX = std::numeric_limits<float>::max();
    m_maxValueX = std::numeric_limits<float>::lowest();
    m_minValueY = std::numeric_limits<float>::max();
    m_maxValueY = std::numeric_limits<float>::lowest();
    m_minValueZ = std::numeric_limits<float>::max();
    m_maxValueZ = std::numeric_limits<float>::lowest();
	m_isValid = false;
	
    m_fieldDimensionY = m_fieldRadius*2;
    m_fieldDimensionZ = m_fieldRadius*2;
    m_bottomZ = m_hubheight - m_fieldRadius;

    m_pointsPerSideZ = m_pointsPerSideY;

	m_cancelCalculation = cancelCalculation;

    /* m_meanWindSpeedAtHub */
    if (m_profileModel == LOG) {
        // calculated with log wind profile.
        m_meanWindSpeedAtHub = m_meanWindSpeed * log((m_hubheight+0) / m_roughnessLength) / log(m_windSpeedMeasurementHeight / m_roughnessLength);
    }
    else if (m_profileModel == PL) {
        // calculated with power law wind profile.
        m_meanWindSpeedAtHub = m_meanWindSpeed * pow (m_hubheight/m_windSpeedMeasurementHeight,m_shearExponent);
    }
    else {
        m_meanWindSpeedAtHub = m_meanWindSpeed;
    }

    m_simulationTime = m_assignedSimulationTime*1.05 + m_fieldDimensionY / m_meanWindSpeedAtHub;
    m_numberOfTimesteps = m_simulationTime / m_assignedTimeStep + 1;
	
    /* m_yCoordinates */
    const float deltaY = m_fieldDimensionY / (m_pointsPerSideY-1);
	//qDebug() << "deltaYZ: " << deltaYZ;
    m_yCoordinates = new float[m_pointsPerSideY];
    for (int i = 0; i < m_pointsPerSideY; ++i) {
        m_yCoordinates[i] = -m_fieldDimensionY/2 + i*deltaY;
        //qDebug() << "yzCoordinates: " << m_yCoordinates[j];
	}

    const float deltaZ = m_fieldDimensionZ / (m_pointsPerSideZ-1);
    m_zCoordinates = new float[m_pointsPerSideZ];
    for (int i = 0; i < m_pointsPerSideZ; ++i) {
        m_zCoordinates[i] = -m_fieldDimensionZ/2 + i*deltaZ;
        //qDebug() << "yzCoordinates: " << m_yCoordinates[j];
    }
	
    /* m_yCoordinatesNormalize */
    m_yCoordinatesNormalized = new float[m_pointsPerSideY];
    for (int i = 0; i < m_pointsPerSideY; ++i) {
        m_yCoordinatesNormalized[i] = m_yCoordinates[i] * 4 / m_fieldDimensionY;
	}	

    m_zCoordinatesNormalized = new float[m_pointsPerSideZ];
    for (int i = 0; i < m_pointsPerSideZ; ++i) {
        m_zCoordinatesNormalized[i] = m_zCoordinates[i] * 4 / m_fieldDimensionZ;
    }
	
	/* m_timeAtTimestep */
	const float deltaT = m_simulationTime/(m_numberOfTimesteps-1);
	//qDebug() << "delta t: " << deltaT;
	m_timeAtTimestep = new float[m_numberOfTimesteps];
	for (int i = 0; i < m_numberOfTimesteps; ++i) {
		m_timeAtTimestep[i] = i * deltaT;
	}

	/* m_meanWindSpeedAtHeigth */
    m_meanWindSpeedAtHeigth = new float[m_pointsPerSideY];
    for (int i = 0; i < m_pointsPerSideY; ++i) {
        if (m_profileModel == LOG) {
			// calculated with log wind profile. Should not be used with heigth above 100m (see wikipedia)
            m_meanWindSpeedAtHeigth[i] = m_meanWindSpeed * log((m_hubheight+m_yCoordinates[i]) / m_roughnessLength) / log(m_windSpeedMeasurementHeight / m_roughnessLength);
        }
        else if (m_profileModel == PL) {
            // calculated with power law wind profile.
            m_meanWindSpeedAtHeigth[i] = m_meanWindSpeed * pow ((m_hubheight+m_yCoordinates[i])/m_windSpeedMeasurementHeight,m_shearExponent);
        }
        else {
			m_meanWindSpeedAtHeigth[i] = m_meanWindSpeed;
		}
	}
	
	/* m_resultantVelocity */
    m_resultantVelocity = new Vec3i**[m_pointsPerSideZ];
    for (int z = 0; z < m_pointsPerSideZ; ++z) {
        m_resultantVelocity[z] = new Vec3i*[m_pointsPerSideY];
        for (int y = 0; y < m_pointsPerSideY; ++y) {
			// empty paranthesis initialize the whole new array with 0
            m_resultantVelocity[z][y] = new Vec3i[m_numberOfTimesteps] ();
		}
	}
}

NewCurve* WindField::newCurve (QString xAxis, QString yAxis, NewGraph::GraphType graphType){

    switch (graphType) {
        case NewGraph::WindfieldTimeGraph:
        {
            if (!m_WindfieldGraphData.size()) return NULL;
            if (!m_WindfieldGraphData.at(0).size()) return NULL;

            const int xAxisIndex = m_availableWindfieldVariables.indexOf(xAxis);
            const int yAxisIndex = m_availableWindfieldVariables.indexOf(yAxis);
            if (xAxisIndex == -1 || yAxisIndex == -1) {
                return NULL;
            }
            else{

                NewCurve *curve = new NewCurve (this);
                curve->setAllPoints(m_WindfieldGraphData[xAxisIndex].data(),
                                    m_WindfieldGraphData[yAxisIndex].data(),
                                    m_WindfieldGraphData.at(0).size());  // numberOfRows is the same for all results
                return curve;
            }
        }
        case NewGraph::PSDGraph:
        {
            if (isShownInGraph()  && !g_windFieldModule->m_bisGlView){
                const int yAxisIndex = m_availableWindfieldVariables.indexOf(yAxis);
                if (yAxisIndex == -1) {
                    return NULL;
                }

                QVector<float> *data = &m_WindfieldGraphData[yAxisIndex];

                QVector<float> xData,yData;

                CalculatePSD(data,xData,yData,m_assignedTimeStep);

                if (xData.size() && yData.size()){

                    NewCurve *curve = new NewCurve (this);
                    curve->setAllPoints(xData.data(),
                                        yData.data(),
                                        yData.size());  // numberOfRows is the same for all results
                    return curve;
                }
            }
        }
        default:
        return NULL;
    }


    return NULL;
}

QStringList WindField::getAvailableVariables (NewGraph::GraphType graphType, bool xAxis){
    switch (graphType) {
    case NewGraph::WindfieldTimeGraph:
        return m_availableWindfieldVariables;
    case NewGraph::PSDGraph:
        if (!xAxis)
            return m_availableWindfieldVariables;
        else{
            QStringList list;
            list.append("Frequency [Hz]");
            return list;
        }
    default:
        return QStringList();
    }
}

WindField::~WindField() {
//	glDeleteLists(m_glListIndex, 1);
    delete [] m_yCoordinates;
    delete [] m_yCoordinatesNormalized;
	delete [] m_timeAtTimestep;
	delete [] m_meanWindSpeedAtHeigth;
	
    for (int z = 0; z < m_pointsPerSideZ; ++z) {
        for (int y = 0; y < m_pointsPerSideY; ++y) {
			delete [] m_resultantVelocity[z][y];
		}
		delete [] m_resultantVelocity[z];
	}
	delete [] m_resultantVelocity;
}

QStringList WindField::prepareMissingObjectMessage() {
	if (g_windFieldStore.isEmpty()) {
		QStringList message;
		if (g_mainFrame->m_iApp == WINDFIELDMODULE) {
			message = QStringList(">>> Click 'New' to create a new Windfield");
		} else {
			message = QStringList(">>> Create a new Windfield in the Windfield Module");
		}
		message.prepend("- No Windfield in Database");
		return message;
	} else {
		return QStringList();
	}
}

void WindField::render(int component) {
	const int grid = 1234, foot = 12345, field = 123456;  // TODO this is incredibly dirty!
	
	if (glIsList(foot)) glDeleteLists(foot,1);
	if (glIsList(field)) glDeleteLists(field,1);
	if (glIsList(grid)) glDeleteLists(grid,1);
	
	int z, y;
	
	hsv hs;
	hs.s = 1.0;
	hs.v = 1.0;
	
	glDisable(GL_POLYGON_SMOOTH);  // NM TODO this probabely should not be here but in the module initialisation
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_LINE_SMOOTH);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	
	glNewList(foot, GL_COMPILE);
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_POLYGON_OFFSET_FILL);  // polygons get a reduced Z-value. Now the grid is drawn onto the WindField surface
	glPolygonOffset(1.0, 0);
	
	/* ground grid with z = -0.1 to avoid zFighting with WindField */
    glColor4d (0.5, 0.5, 0.5, globalLineAlpha/2.0);
    glBegin(GL_LINES);

    glLineWidth(0.5);


    double top = m_hubheight + 0.5*std::min(m_fieldDimensionY,m_fieldDimensionZ);
    double bot = top - m_fieldDimensionZ;

    double maxdim = std::max(m_fieldDimensionY,m_fieldDimensionZ);

    double scaleY = m_fieldDimensionY/maxdim;
    double scaleZ = m_fieldDimensionZ/maxdim;

    for (z = 0; z < m_pointsPerSideZ; z++) {
        for (y = 0; y < m_pointsPerSideY; y++) {
        glVertex3f (m_yCoordinatesNormalized[y] * scaleY, m_zCoordinatesNormalized[0]  * scaleZ, -0.1);  // vertical
        glVertex3f (m_yCoordinatesNormalized[y] * scaleY, m_zCoordinatesNormalized[m_pointsPerSideZ-1]  * scaleZ, -0.1);
        glVertex3f (m_yCoordinatesNormalized[0] * scaleY, m_zCoordinatesNormalized[z]  * scaleZ, -0.1);  // horizontal
        glVertex3f (m_yCoordinatesNormalized[m_pointsPerSideY-1] * scaleY, m_zCoordinatesNormalized[z]  * scaleZ, -0.1);
        }
	}
	glEnd();
	
	/* grid foot for signaling floor side of the field */
    glBegin (GL_LINES);
    for (z = -20; z <= 20; ++z) {
        glVertex3f (m_yCoordinatesNormalized[0]  * scaleY, (m_zCoordinatesNormalized[0] - 4.0*bot/m_fieldDimensionZ) * scaleZ, z/20.0 * scaleZ - 0.1);
        glVertex3f (m_yCoordinatesNormalized[m_pointsPerSideY-1]  * scaleY, (m_zCoordinatesNormalized[0] - 4.0*bot/m_fieldDimensionZ)  * scaleZ, z/20.0 * scaleZ - 0.1);
	}
	glEnd();
	glEndList();
	
	glNewList(field, GL_COMPILE);

	/* the windfield */
    float difference;
    if (component == 0) difference = m_maxValueX - m_minValueX;
    if (component == 1) difference = m_maxValueY - m_minValueY;
    if (component == 2) difference = m_maxValueZ - m_minValueZ;

	float normalized;
    for (z = 0; z < m_pointsPerSideZ - 1; ++z) {

		glBegin(GL_TRIANGLE_STRIP);  // the surface
		glEnable(GL_POLYGON_OFFSET_FILL);  // polygons get a reduced Z-value. Now the grid is drawn onto the WindField surface
		glPolygonOffset(1.0, 0);
        for (y = 0; y < m_pointsPerSideY; ++y) {

			/* normalized to [0,1] */
            if (component == 0) normalized = (convertX(m_resultantVelocity[z][y][m_shownTimestep].x) - m_minValueX) / difference;
            if (component == 1) normalized = (convertY(m_resultantVelocity[z][y][m_shownTimestep].y) - m_minValueY) / difference;
            if (component == 2) normalized = (convertZ(m_resultantVelocity[z][y][m_shownTimestep].z) - m_minValueZ) / difference;
//            normalized = (convertX(m_resultantVelocity[z][y][m_shownTimestep].x)) / m_maxValueX;

			hs.h = (1-normalized)*225;
			
			
			glColor3f (hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b);
            glVertex3f (m_yCoordinatesNormalized[y] * scaleY, m_zCoordinatesNormalized[z] * scaleZ, 2*normalized*scaleZ);
			
			
            if (component == 0) normalized = (convertX(m_resultantVelocity[z+1][y][m_shownTimestep].x) - m_minValueX) / difference;
            if (component == 1) normalized = (convertY(m_resultantVelocity[z+1][y][m_shownTimestep].y) - m_minValueY) / difference;
            if (component == 2) normalized = (convertZ(m_resultantVelocity[z+1][y][m_shownTimestep].z) - m_minValueZ) / difference;
//            normalized = (convertX(m_resultantVelocity[z+1][y][m_shownTimestep].x)) / m_maxValueX;

			hs.h = (1-normalized)*225;
			
			glColor3f (hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b);
            glVertex3f (m_yCoordinatesNormalized[y] * scaleY, m_zCoordinatesNormalized[z+1] * scaleZ, 2*normalized*scaleZ);
		}
		glEnd();
	}
	glEndList();
	
	glNewList(grid, GL_COMPILE);

    for (z = 0; z < m_pointsPerSideZ - 1; ++z) {
        glColor4d (0, 0, 0, globalLineAlpha/2.0);
		glBegin(GL_LINE_STRIP);  // the zigzag lines on the surface
		glEnable(GL_POLYGON_OFFSET_FILL);  // polygons get a reduced Z-value. Now the grid is drawn onto the WindField surface
		glPolygonOffset(-2, -2);
        for (y = 0; y < m_pointsPerSideY; ++y) {
            if (component == 0) normalized = (convertX(m_resultantVelocity[z][y][m_shownTimestep].x) - m_minValueX) / difference;
            if (component == 1) normalized = (convertY(m_resultantVelocity[z][y][m_shownTimestep].y) - m_minValueY) / difference;
            if (component == 2) normalized = (convertZ(m_resultantVelocity[z][y][m_shownTimestep].z) - m_minValueZ) / difference;
//            normalized = (convertX(m_resultantVelocity[z][y][m_shownTimestep].x)) / m_maxValueX;
            glVertex3f (m_yCoordinatesNormalized[y] * scaleY, m_zCoordinatesNormalized[z] * scaleZ, 2*normalized*scaleZ);

            if (component == 0) normalized = (convertX(m_resultantVelocity[z+1][y][m_shownTimestep].x) - m_minValueX) / difference;
            if (component == 1) normalized = (convertY(m_resultantVelocity[z+1][y][m_shownTimestep].y) - m_minValueY) / difference;
            if (component == 2) normalized = (convertZ(m_resultantVelocity[z+1][y][m_shownTimestep].z) - m_minValueZ) / difference;
//            normalized = (convertX(m_resultantVelocity[z+1][y][m_shownTimestep].x)) / m_maxValueX;
            glVertex3f (m_yCoordinatesNormalized[y] * scaleY, m_zCoordinatesNormalized[z+1] * scaleZ, 2*normalized*scaleZ);
		}
		glEnd();
		
	}
	
    glColor4d (0, 0, 0, globalLineAlpha/2.0);
    for (z = 0; z < m_pointsPerSideZ; ++z) {
		glBegin(GL_LINE_STRIP);  // the straigth lines 
		glEnable(GL_POLYGON_OFFSET_FILL);  // polygons get a reduced Z-value. Now the grid is drawn onto the WindField surface
		glPolygonOffset(-2, -2);
        for (y = 0; y < m_pointsPerSideY; ++y) {
			/* normalized to [0,1] */
            if (component == 0) normalized = (convertX(m_resultantVelocity[z][y][m_shownTimestep].x) - m_minValueX) / difference;
            if (component == 1) normalized = (convertY(m_resultantVelocity[z][y][m_shownTimestep].y) - m_minValueY) / difference;
            if (component == 2) normalized = (convertZ(m_resultantVelocity[z][y][m_shownTimestep].z) - m_minValueZ) / difference;
//            normalized = (convertX(m_resultantVelocity[z][y][m_shownTimestep].x)) / m_maxValueX;
            glVertex3f (m_yCoordinatesNormalized[y] * scaleY, m_zCoordinatesNormalized[z] * scaleZ, 2*normalized*scaleZ);
		}
		glEnd ();
	}
	glEndList();
	
	glCallList(foot);
	glCallList(field);
	glCallList(grid);

}

void WindField::renderForQLLTSim(double time, double dist, double radius, double mean, bool redblue, int GlList, bool mirror, bool autoShift, double shiftTime) {
    int z, y;

    double depth = radius;

    if (autoShift) time += m_fieldDimensionY/2/m_meanWindSpeedAtHub;
    else time += shiftTime;

    if (time < 0) time = 0;

    if (mirror){
        time = fabs(time);

        while (time > 2.0*m_simulationTime)
            time -= 2.0*m_simulationTime;


        if (time > m_simulationTime)
            time = 2.0*m_simulationTime - time;

    }
    else{
        while (time > m_simulationTime)
            time -= m_simulationTime;

    }


    double deltaT = m_simulationTime/(m_numberOfTimesteps-1);
    int t = floor(time/deltaT);

    if (t > (m_numberOfTimesteps-2)){
        time = m_simulationTime/(m_numberOfTimesteps-1)*(m_numberOfTimesteps-2);
        t = (m_numberOfTimesteps-2);
    }

    double c = (time - deltaT*t)/deltaT;

    if (c < 10e-5) c = 0;

    hsv hs;
    hs.s = 1.0;
    hs.v = 1.0;

    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    glNewList(GlList, GL_COMPILE);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_POLYGON_OFFSET_FILL);  // polygons get a reduced Z-value. Now the grid is drawn onto the WindField surface
    glPolygonOffset(1.0, 0);

    /* ground grid with z = -0.1 to avoid zFighting with WindField */
    glColor4d (0.5, 0.5, 0.5, globalLineAlpha/2.0);
    glBegin(GL_LINES);

    for (y = 0; y < m_pointsPerSideY; y++) {
        glVertex3f (-0.1+dist,m_yCoordinates[y], m_zCoordinates[0]+m_fieldDimensionZ/2.+m_bottomZ);  // vertical
        glVertex3f (-0.1+dist,m_yCoordinates[y], m_zCoordinates[m_pointsPerSideZ-1]+m_fieldDimensionZ/2.+m_bottomZ);
    }

    for (z = 0; z < m_pointsPerSideZ; z++) {
        glVertex3f (-0.1+dist,m_yCoordinates[0], m_zCoordinates[z]+m_fieldDimensionZ/2.+m_bottomZ);  // horizontal
        glVertex3f (-0.1+dist,m_yCoordinates[m_pointsPerSideY-1], m_zCoordinates[z]+m_fieldDimensionZ/2.+m_bottomZ);
    }

    glEnd();


    double fac = 1.3;

    /* the windfield */
    float normalized;
    double vel;
    for (z = 0; z < m_pointsPerSideZ - 1; ++z) {
        glBegin(GL_TRIANGLE_STRIP);  // the surface
        glEnable(GL_POLYGON_OFFSET_FILL);  // polygons get a reduced Z-value. Now the grid is drawn onto the WindField surface
        glPolygonOffset(1.0, 0);
        for (y = 0; y < m_pointsPerSideY; ++y) {
            /* normalized to [0,1] */

            normalized = (1-c)*convertX(m_resultantVelocity[z][y][t].x) / m_maxValueX  + (c)*convertX(m_resultantVelocity[z][y][t+1].x) / m_maxValueX ;
            vel = fabs((1-c)*convertX(m_resultantVelocity[z][y][t].x) / mean + (c)*convertX(m_resultantVelocity[z][y][t+1].x) / mean );
            if (!redblue){
            hs.h = (1-vel/fac)*225;
            glColor4d (hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
            }
            else{

                if (vel > 1) glColor4d(1,0,0,vel-1);
                else glColor4d(0,0,1,1-vel);
            }
            glVertex3f (depth*normalized+dist, m_yCoordinates[y], m_zCoordinates[z]+m_fieldRadius+m_bottomZ);

            normalized = (1-c)*convertX(m_resultantVelocity[z+1][y][t].x) / m_maxValueX  + (c)*convertX(m_resultantVelocity[z+1][y][t+1].x) / m_maxValueX ;
            vel = fabs((1-c)*convertX(m_resultantVelocity[z+1][y][t].x) / mean  + (c)*convertX(m_resultantVelocity[z+1][y][t+1].x) / mean );

            if (!redblue){
            hs.h = (1-vel/fac)*225;
            glColor4d (hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
            }
            else{
                if (vel > 1) glColor4d(1,0,0,vel-1);
                else glColor4d(0,0,1,1-vel);
            }
            glVertex3f (depth*normalized+dist, m_yCoordinates[y], m_zCoordinates[z+1]+m_fieldRadius+m_bottomZ);
        }
        glEnd();
    }




    for (z = 0; z < m_pointsPerSideZ - 1; ++z) {
        glColor4d (0, 0, 0, globalLineAlpha/2.0);
        glBegin(GL_LINE_STRIP);  // the zigzag lines on the surface
        glEnable(GL_POLYGON_OFFSET_FILL);  // polygons get a reduced Z-value. Now the grid is drawn onto the WindField surface
        glPolygonOffset(-2, -2);
        for (y = 0; y < m_pointsPerSideY; ++y) {
            normalized = (1-c)*convertX(m_resultantVelocity[z][y][t].x) / m_maxValueX + (c)*convertX(m_resultantVelocity[z][y][t+1].x) / m_maxValueX;

            glVertex3f (depth*normalized+dist, m_yCoordinates[y], m_zCoordinates[z]+m_fieldRadius+m_bottomZ);
            normalized = fabs((1-c)*convertX(m_resultantVelocity[z+1][y][t].x) / m_maxValueX+(c) * convertX(m_resultantVelocity[z+1][y][t+1].x) / m_maxValueX);

            glVertex3f (depth*normalized+dist, m_yCoordinates[y], m_zCoordinates[z+1]+m_fieldRadius+m_bottomZ);
        }
        glEnd();

    }

    glColor4d (0, 0, 0, globalLineAlpha/2.0);
    for (z = 0; z < m_pointsPerSideZ; ++z) {
        glBegin(GL_LINE_STRIP);  // the straigth lines
        glEnable(GL_POLYGON_OFFSET_FILL);  // polygons get a reduced Z-value. Now the grid is drawn onto the WindField surface
        glPolygonOffset(-2, -2);
        for (y = 0; y < m_pointsPerSideY; ++y) {
            /* normalized to [0,1] */
            normalized = (1-c)*convertX(m_resultantVelocity[z][y][t].x) / m_maxValueX + (c)*convertX(m_resultantVelocity[z][y][t+1].x) / m_maxValueX;

            glVertex3f (depth*normalized+dist, m_yCoordinates[y], m_zCoordinates[z]+m_fieldRadius+m_bottomZ);
        }
        glEnd ();
    }
    glEndList();
}

void WindField::exportToBinary(QDataStream &dataStream) {
	dataStream.setByteOrder(QDataStream::LittleEndian);  // AeroDyn expects this
	dataStream.setFloatingPointPrecision(QDataStream::SinglePrecision);  // float32
	
	dataStream << qint16(7) <<  // ID: Identifies the file as a TurbSim binary file
                  qint32(m_pointsPerSideZ) <<  // NumGrid_Z: The number of grid points in the vertical direction
                  qint32(m_pointsPerSideY) <<  // NumGrid_Y: The number of grid points in the horizontal direction
				  qint32(0) <<  // n_tower: The number of tower points below the grid
				  qint32(m_numberOfTimesteps) <<  // n_t: The number of time steps
                  qreal(m_fieldDimensionZ / (m_pointsPerSideZ-1)) <<  // dz: The distance in meters between two adjacent points in the vertical direction
                  qreal(m_fieldDimensionY / (m_pointsPerSideY-1)) <<  // dy: The distance in meters between two adjacent points in the horizontal direction.
				  qreal(m_simulationTime / (m_numberOfTimesteps-1)) <<  // TimeStep: The time in seconds between consecutive grids
				  qreal(m_meanWindSpeedAtHub) <<  // u_hub: The mean wind speed in m/s at hub height
				  qreal(m_hubheight) <<  // HubHt: The height in meters of the hub
                  qreal(m_hubheight - m_fieldRadius);  // Z_bottom: The height in meters of the bottom of the grid

	/* calculate V_slope and V_intercept
	 * In the binary file the values for velocity are compressed to int16. According to the manuel the formular is:
	 * V_grid = (V_grid_norm - V_intercept) / V_slope
	 * where V_grid is the original value and V_grid_norm is the normalized int16 value.
	 * The TurbSim code (Fortran) looks as follows:
	 * UScl = IntRng/REAL( VMax(1) - VMin(1) , SiKi )
	 * UOff = IntMin - UScl*REAL( VMin(1)    , SiKi )
	 * */
//    const float valueRangeX = m_maxValueX - m_minValueX;
//    const float V_slopeX = 65535 / valueRangeX;
//    const float V_interceptX = -32768 - V_slopeX*m_minValueX;

//    const float valueRangeY = m_maxValueY - m_minValueY;
//    const float V_slopeY = 65535 / valueRangeY;
//    const float V_interceptY = -32768 - V_slopeY*m_minValueY;

//    const float valueRangeZ = m_maxValueZ - m_minValueZ;
//    const float V_slopeZ = 65535 / valueRangeZ;
//    const float V_interceptZ = -32768 - V_slopeZ*m_minValueZ;

    dataStream << qreal(vslopeX) <<  // V_slope_X
                  qreal(vinterceptX) <<  // V-intercept_X
                  qreal(vslopeY) <<  // V_slope_Y
                  qreal(vinterceptY) <<  // V-intercept_Y
                  qreal(vslopeZ) <<  // V_slope_Z
                  qreal(vinterceptZ);  // V-intercept_Z
	
	/* write the info text */
	QString infoString ("Full-field file for " + m_objectName + ". Generated by QBlade.");
	QByteArray infoByteArray = infoString.toLatin1();
	dataStream << qint32(infoByteArray.size());
	for (int i = 0; i < infoByteArray.size(); i++) {
		dataStream << qint8(infoByteArray[i]);
	}
	
    /* write the velocity values */
	for (int timestep = 0; timestep < m_numberOfTimesteps; ++timestep) {
        for (int zIndex = 0; zIndex < m_pointsPerSideZ; ++zIndex) {
            for (int yIndex = 0; yIndex < m_pointsPerSideY; ++yIndex) {
                dataStream << qint16(m_resultantVelocity[zIndex][yIndex][timestep].x) <<
                              qint16(m_resultantVelocity[zIndex][yIndex][timestep].y) <<
                              qint16(m_resultantVelocity[zIndex][yIndex][timestep].z);
			}
		}
	}
}

void WindField::importFromBinary(QDataStream &dataStream) {
    qint8 q8;
    qint16 q16;
    qint32 q32;
    qreal qr;

    dataStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    dataStream >> q16; // placeholder for "7" identifier

    dataStream >> q32;
    m_pointsPerSideZ = q32;
    dataStream >> q32;
    m_pointsPerSideY = q32;

    dataStream >> q32; // placeholder for tower points

    dataStream >> q32;
    m_numberOfTimesteps = q32;
    dataStream >> qr;

    m_fieldDimensionZ = qr * (m_pointsPerSideZ-1);

    dataStream >> qr;

    m_fieldDimensionY = qr * (m_pointsPerSideY-1);
    dataStream >> qr;
    m_simulationTime = qr * (m_numberOfTimesteps-1);
    dataStream >> qr;
    m_meanWindSpeedAtHub = qr;
    dataStream >> qr;
    m_hubheight = qr;
    dataStream >> qr;
    m_bottomZ = qr;

    // initialize vector matrix
    m_resultantVelocity = new Vec3i**[m_pointsPerSideZ];
    for (int z = 0; z < m_pointsPerSideZ; ++z) {
        m_resultantVelocity[z] = new Vec3i*[m_pointsPerSideY];
        for (int y = 0; y < m_pointsPerSideY; ++y) {
            m_resultantVelocity[z][y] = new Vec3i[m_numberOfTimesteps];
        }
    }

    double range;

    dataStream >> qr;
    vslopeX = qr;

    dataStream >> qr;
    vinterceptX = qr;

    dataStream >> qr;
    vslopeY = qr;

    dataStream >> qr;
    vinterceptY = qr;

    dataStream >> qr;
    vslopeZ = qr;

    dataStream >> qr;
    vinterceptZ = qr;


    range = 65535/vslopeX;

    m_minValueX = (-1)*(vinterceptX+32768)/vslopeX;

    m_maxValueX = m_minValueX+range;


    range = 65535/vslopeY;

    m_minValueY = (-1)*(vinterceptY+32768)/vslopeY;

    m_maxValueY = m_minValueY+range;

    range = 65535/vslopeZ;

    m_minValueZ = (-1)*(vinterceptZ+32768)/vslopeZ;

    m_maxValueZ = m_minValueZ+range;


    dataStream >> q32;
    int size= q32;

    for (int i=0; i<size; i++){
        dataStream >> q8;
    }

    for (int timestep = 0; timestep < m_numberOfTimesteps; ++timestep) {
        for (int zIndex = 0; zIndex < m_pointsPerSideZ; ++zIndex) {
            for (int yIndex = 0; yIndex < m_pointsPerSideY; ++yIndex) {
                dataStream >> q16;
                m_resultantVelocity[zIndex][yIndex][timestep].x = q16;
                dataStream >> q16;
                m_resultantVelocity[zIndex][yIndex][timestep].y = q16;
                dataStream >> q16;
                m_resultantVelocity[zIndex][yIndex][timestep].z = q16;
            }
        }
    }

    // initialize

    /* m_yCoordinates */
    const float deltaY = m_fieldDimensionY / (m_pointsPerSideY-1);
    //qDebug() << "deltaYZ: " << deltaYZ;
    m_yCoordinates = new float[m_pointsPerSideY];
    for (int i = 0; i < m_pointsPerSideY; ++i) {
        m_yCoordinates[i] = -m_fieldDimensionY/2 + i*deltaY;
//        qDebug() << "yzCoordinates: " << m_yCoordinates[i];
    }

    /* m_yCoordinates */
    const float deltaZ = m_fieldDimensionZ / (m_pointsPerSideZ-1);
    //qDebug() << "deltaYZ: " << deltaYZ;
    m_zCoordinates = new float[m_pointsPerSideZ];
    for (int i = 0; i < m_pointsPerSideZ; ++i) {
        m_zCoordinates[i] = -m_fieldDimensionZ/2 + i*deltaZ;
//        qDebug() << "yzCoordinates: " << m_yCoordinates[i];
    }

    /* m_yCoordinatesNormalize */
    m_yCoordinatesNormalized = new float[m_pointsPerSideY];
    for (int i = 0; i < m_pointsPerSideY; ++i) {
        m_yCoordinatesNormalized[i] = m_yCoordinates[i] * 4 / m_fieldDimensionY;
//        qDebug() << "m_yCoordinatesNormalized: " << m_yCoordinatesNormalized[i];
    }

    /* m_yCoordinatesNormalize */
    m_zCoordinatesNormalized = new float[m_pointsPerSideZ];
    for (int i = 0; i < m_pointsPerSideZ; ++i) {
        m_zCoordinatesNormalized[i] = m_zCoordinates[i] * 4 / m_fieldDimensionZ;
//        qDebug() << "m_yCoordinatesNormalized: " << m_yCoordinatesNormalized[i];
    }

    /* m_timeAtTimestep */
    const float deltaT = m_simulationTime/(m_numberOfTimesteps-1);
    //qDebug() << "delta t: " << deltaT;
    m_timeAtTimestep = new float[m_numberOfTimesteps];
    for (int i = 0; i < m_numberOfTimesteps; ++i) {
        m_timeAtTimestep[i] = i * deltaT;

//        qDebug () << "Timesteps: " << m_timeAtTimestep[i];
    }

    /* m_meanWindSpeedAtHeigth */
    m_meanWindSpeedAtHeigth = new float[m_pointsPerSideZ];
    for (int i = 0; i < m_pointsPerSideZ; ++i) {
            m_meanWindSpeedAtHeigth[i] = m_meanWindSpeed;
    }



    m_meanWindSpeed = m_meanWindSpeedAtHub;
    m_windSpeedMeasurementHeight = m_hubheight;
	m_turbulenceIntensity = 1;
    m_fieldRadius = m_fieldDimensionZ / 2;

	m_roughnessLength = 0.1;

    m_assignedTimeStep = m_simulationTime/(m_numberOfTimesteps-1);

    m_bisTurbSim = true;
    m_bisImported = true;

    m_isValid = true;

}

void WindField::exportToTxt(QTextStream &stream) {
    ExportFileHeader(stream);
    stream <<
              "# Timesteps: " << m_numberOfTimesteps << endl <<
              "# Temporal Stepwidth: " << m_simulationTime / (m_numberOfTimesteps-1) << " s" << endl <<
              "# Points per SideY: " << m_pointsPerSideY << endl <<
              "# Spatial StepwidthY: " << m_fieldDimensionY / (m_pointsPerSideY-1) << " m" << endl <<
              "# Points per SideZ: " << m_pointsPerSideZ << endl <<
              "# Spatial StepwidthZ: " << m_fieldDimensionZ / (m_pointsPerSideZ-1) << " m" << endl <<
              "# Hub Height: " << m_hubheight << " m" << endl <<
              "# Mean Wind Speed at Hub: " << m_meanWindSpeedAtHub << " m/s" << endl <<
              "# Heigth of lowest Point: " << m_hubheight - m_fieldRadius << " m" << endl <<
			  endl;
			  
	for (int timestep = 0; timestep < m_numberOfTimesteps; ++timestep) {
        for (int zIndex = 0; zIndex < m_pointsPerSideZ; ++zIndex) {
            for (int yIndex = 0; yIndex < m_pointsPerSideY; ++yIndex) {
                stream << QString("%1 %2 %3").arg(convertX(m_resultantVelocity[zIndex][yIndex][timestep].x), 7, 'f', 3).arg(convertY(m_resultantVelocity[zIndex][yIndex][timestep].y), 7, 'f', 3).arg(convertZ(m_resultantVelocity[zIndex][yIndex][timestep].z), 7, 'f', 3);
			}
			stream << endl;
		}
		stream << endl;
	}
}

void WindField::serialize() {
	StorableObject::serialize();
    ShowAsGraphInterface::serialize();

	if (g_serializer.isReadMode()) {
		m_glListIndex = glGenLists(1);
	}
	g_serializer.readOrWriteInt (&m_shownTimestep);
    g_serializer.readOrWriteFloat (&m_minValueX);
    g_serializer.readOrWriteFloat (&m_maxValueX);
    g_serializer.readOrWriteFloat (&m_minValueY);
    g_serializer.readOrWriteFloat (&m_maxValueY);
    g_serializer.readOrWriteFloat (&m_minValueZ);
    g_serializer.readOrWriteFloat (&m_maxValueZ);
    g_serializer.readOrWriteFloat (&vinterceptX);
    g_serializer.readOrWriteFloat (&vslopeX);
    g_serializer.readOrWriteFloat (&vinterceptY);
    g_serializer.readOrWriteFloat (&vslopeY);
    g_serializer.readOrWriteFloat (&vinterceptZ);
    g_serializer.readOrWriteFloat (&vslopeZ);
	g_serializer.readOrWriteBool (&m_isValid);
	
	/* geometric parameters */
    g_serializer.readOrWriteFloat (&m_fieldRadius);
	g_serializer.readOrWriteFloat (&m_hubheight);
    g_serializer.readOrWriteFloat (&m_fieldDimensionY);
	
	/* wind parameters */
	g_serializer.readOrWriteFloat (&m_meanWindSpeed);
	g_serializer.readOrWriteFloat (&m_turbulenceIntensity);
	g_serializer.readOrWriteFloat (&m_roughnessLength);
	g_serializer.readOrWriteFloat (&m_windSpeedMeasurementHeight);
	
	/* calculation parameters */
	g_serializer.readOrWriteFloat (&m_simulationTime);
	g_serializer.readOrWriteInt (&m_numberOfTimesteps);
    g_serializer.readOrWriteInt (&m_pointsPerSideY);
    g_serializer.readOrWriteInt (&m_pointsPerSideZ);
    g_serializer.readOrWriteFloat (&m_fieldDimensionZ);
    g_serializer.readOrWriteFloat (&m_bottomZ);
    g_serializer.readOrWriteFloat (&m_assignedSimulationTime);
    g_serializer.readOrWriteFloat (&m_assignedTimeStep);

	/* dynamic calculation results */
    g_serializer.readOrWriteFloatArray1D (&m_yCoordinates, m_pointsPerSideY);
    g_serializer.readOrWriteFloatArray1D (&m_yCoordinatesNormalized, m_pointsPerSideY);
    g_serializer.readOrWriteFloatArray1D (&m_zCoordinates, m_pointsPerSideZ);
    g_serializer.readOrWriteFloatArray1D (&m_zCoordinatesNormalized, m_pointsPerSideZ);
	g_serializer.readOrWriteFloatArray1D (&m_timeAtTimestep, m_numberOfTimesteps);
    g_serializer.readOrWriteFloatArray1D (&m_meanWindSpeedAtHeigth, m_pointsPerSideZ);
	g_serializer.readOrWriteFloat (&m_meanWindSpeedAtHub);
    g_serializer.readOrWriteCVectoriArray3D (&m_resultantVelocity, m_pointsPerSideZ, m_pointsPerSideY, m_numberOfTimesteps);

    /* new parameters needed for turb-sim windfields */
    g_serializer.readOrWriteFloat (&m_jetHeight);
    g_serializer.readOrWriteFloat (&m_shearExponent);
    g_serializer.readOrWriteFloat (&m_VRef);
    g_serializer.readOrWriteFloat (&m_hInflow);
    g_serializer.readOrWriteFloat (&m_vInflow);
    g_serializer.readOrWriteFloat (&m_ETMc);
    g_serializer.readOrWriteBool (&m_bisTurbSim);
    g_serializer.readOrWriteBool (&m_bisImported);
    g_serializer.readOrWriteBool (&m_bDefaultShear);
    g_serializer.readOrWriteInt (&m_windModel);
    g_serializer.readOrWriteInt (&m_spectralModel);
    g_serializer.readOrWriteInt (&m_profileModel);
    g_serializer.readOrWriteInt (&m_IEAType);
    g_serializer.readOrWriteInt (&m_turbineClass);
    g_serializer.readOrWriteInt (&m_turbulenceClass);
    g_serializer.readOrWriteInt (&m_seed);

    /* list for probe locations */
    g_serializer.readOrWriteCVectorVector1D(&m_probeLocations);

}

void WindField::calculateWindField() {
	/* The turbulent windfield calculation is based on the model of Veers. To understand the following code
     * it is essential to read the Sandia report "Three-Dimensional Wind Simulation", 1988, or review
	 * Chapter 14 of "Aerodynamics of Wind Turbines 2nd edition" from M. O.L. Hansen.
	 *
	 * This functions creates a 3 dimensional matrix, where the first 2 dimensions refer to the points in
	 * space, and the third dimension is the time. The spatial distribution of the points is equidistant and
	 * square, expressed in a cartesian coordinate system with it's point of origin in the middle of the
	 * windfield.
	 * The windfield is divided into jxk (j=k) points in space. The points are numbered like this (view from
	 * the front to the turbine):
	 *   7 8 9
	 *   4 5 6
	 *   1 2 3
	 * The coordinate (so z is the heigth):
	 *  z ^
	 *    |
	 *    | --> y
	 * */
	
	int j;  // j and k refer both to all discrete points of the windfield
	int k;
	int m;  // refers to all the frequencies
	float sum;  // used for several summations
	
    const int pointsInTotal = m_pointsPerSideZ * m_pointsPerSideY;  // total number of points in windflied
	const int numberOfFrequencies = m_numberOfTimesteps / 2;  // see Hansen p. 152
	const float deltaF = 1 / m_simulationTime;  // frequency steps in [Hz]
		
	float* frequency;  // contains all calculated N/2 frequencies
	
	/* Matrix H: According to the number of points this Matrix can get very big. There shouldn't be more
	 * than around 100 points in one direction, as the size of this matrix is 100^2 x 100^2 ( /2, as the
	 * matrix is lower triangular).
	 * The Matrix H represents the weighted factors for the linear combination of N independant,
	 * unit-magnitude, white noise inputs, that will yield in N correlated outputs with the correct spectral
	 * matrix. Each row of H gives the contributions of all the inputs to the outputpoint at k.
	 * Each column gives the contributions of the j^(th) input to all of the outputs
	 * */
	float **H;
	
	/* Matrix S is the spectral matrix. The diagonals of S are the PSDs. Each off-diagonal term S_jk is
	 * the cross spectral density between points j and k.
	 * To save memory there is only need for the k inputs (columns) for each j (row). Therefore S_j is
	 * only a 1D Vector foreach current j. Additionally there are the diagonal terms needed
	 * */
	float *S_j  = NULL;
	float *S_diagonal = NULL;
	
	/* Check Hansen for additional information */
	float **amplitude = NULL;
	float **phi = NULL;
	
	/* Random matrix */
	float **random = NULL;
    srand (m_seed);
	//srand (0);  // for testing: gives always the same sequence of random numbers
	
	/* In Veers a factor deltaF/2 is used to normalize the PSD, while in Hansen the integral of
	 * the PSD over all calculated frequencys must be one. In general Hansens method should be
	 * more accurate but at all test deltaF/2 made the resultant turbulense intensity to fit better
	 * with the value in the setup
	 * */
	float *psdNormalizationFactor = NULL;
	
	
	/* * * * * * allocate thread shared memory * * * * * */
	frequency = new float[numberOfFrequencies];
	for (m = 0; m < numberOfFrequencies; ++m) {
		frequency[m] = (m+1) / m_simulationTime;
	}
	
	amplitude = new float*[pointsInTotal];
	for (j = 0; j < pointsInTotal; ++j) {
		amplitude[j] = new float[numberOfFrequencies] ();
	}
	
	phi = new float*[pointsInTotal];
	for (j = 0; j < pointsInTotal; ++j) {
		phi[j] = new float[numberOfFrequencies] ();
	}
	
	random = new float*[pointsInTotal];
	for (j = 0; j < pointsInTotal; ++j) {
		random[j] = new float[numberOfFrequencies];
		for (m = 0; m < numberOfFrequencies; ++m) {
            random[j][m] = (rand()*1.0)/RAND_MAX * 2 * PI_;  // values from 0 to 2*Pi
		}
	}
	
    psdNormalizationFactor = new float[m_pointsPerSideZ];
    for (int zIndex = 0; zIndex < m_pointsPerSideZ; ++zIndex) {
//		sum = 0;
//		for (m = 0; m < numberOfFrequencies; ++m) {
//			sum += getPSD(frequency[m], zIndex);
//		}
//		psdNormalizationFactor[zIndex] = 1 / sum;
		psdNormalizationFactor[zIndex] = deltaF / 2;
//		qDebug() << "deltaF/2: " << deltaF/2 << " factor: " << psdNormalizationFactor[zIndex];
    }

    #pragma omp parallel private (j, k, sum, S_j, S_diagonal, H) shared (frequency, random, amplitude, phi, psdNormalizationFactor)
    {
		/* * * * * * allocate thread private memory * * * * * */
		int j_y;  // the column in the windfield of point j
		int j_z;  // the row (or height) in the windfield of point j
		int k_y;
		int k_z;
		int l;  // refers to the column count within the H matrix
		float Re;
		float Im;
		
		S_j = new float[pointsInTotal] ();
		S_diagonal = new float[pointsInTotal] ();
		
		H = new float*[pointsInTotal];
		for (j = 0; j < pointsInTotal; ++j) {
			H[j] = new float[j+1] ();  // lower triangular matrix
        }

		/* * * * * * calculation * * * * * */
        #pragma omp for
		for (m = 0; m < numberOfFrequencies; ++m) {  // independent loop, therefore parallelized
			j_z = 0; j_y = 0;
			for (j = 0; j < pointsInTotal && ! *m_cancelCalculation; ++j) {
				if (j_y == 0) {  // calculate new PSD only at new height
					S_diagonal[j] = getPSD(frequency[m], j_z) * psdNormalizationFactor[j_z];
				} else {  // otherwise take the last one, because it's the same
					S_diagonal[j] = S_diagonal[j-1];
				}
				
				k_z = 0; k_y = 0;
				Re = 0; Im = 0;
				for (k = 0; k <= j; ++k) {
					if (j != k) {  // calculate S_jk
                        S_j[k] = getCoh (frequency[m], getDist(m_yCoordinates[j_y], m_zCoordinates[j_z],
                                                               m_yCoordinates[k_y], m_zCoordinates[k_z] )
																	) * sqrt(S_diagonal[j] * S_diagonal[k]);
					} else {  // calculate S_kk
						S_j[k] = S_diagonal[k];
					}
					sum = 0;
					if (j != k) {  // calculate H_jk
						for (l = 0; l <= k-1; ++l) {
							sum = H[j][l]*H[k][l] + sum;
						}
						H[j][k] = (S_j[k] - sum) / H[k][k];
					} else {  // calculate H_kk
						for (l = 0; l <= k-1; ++l) {
							sum = pow(H[k][l],2) + sum;
						}
						H[k][k] = sqrt(S_j[k] - sum);
					}
					
					Re = Re + (H[j][k]*cos(random[k][m]));
					Im = Im + (H[j][k]*sin(random[k][m]));
					++k_y;
                    if (k_y == m_pointsPerSideY) {
						k_y = 0;
						++k_z;
					}
				}  // for k
				
				amplitude[j][m] = sqrt (pow(Re,2) + pow(Im,2));
				phi[j][m] = atan2 (Im, Re);
				
				emit updateProgress();
				++j_y;
                if (j_y == m_pointsPerSideY) {
					j_y = 0;
					++j_z;
				}
			}  // for j
		}  // for m
		
		/* * * * * * free thread private memory * * * * * */
		delete [] S_j;
		delete [] S_diagonal;
		for (j = 0; j < pointsInTotal; ++j) {
			delete [] H[j];
		}
		delete [] H;
    }  // omp parallel END


    Vec3f*** tempVelocity = new Vec3f**[m_pointsPerSideZ];
    for (int z = 0; z < m_pointsPerSideZ; ++z) {
        tempVelocity[z] = new Vec3f*[m_pointsPerSideY];
        for (int y = 0; y < m_pointsPerSideY; ++y) {
            tempVelocity[z][y] = new Vec3f[m_numberOfTimesteps];
        }
    }

	/* * * * * * superposition of frequencies * * * * * */
	if (! *m_cancelCalculation)
	{
		int z = 0;  // the row (or height) in the windfield
		int y = 0;  // the column in the windfield
		int t = 0;  // refers to the timesteps
		for (j = 0; j < pointsInTotal && ! *m_cancelCalculation; ++j) {  // for every point j
			for (t = 0; t < m_numberOfTimesteps; ++t) {  // for every timestep t
				sum = 0;
				for (m = 0; m < numberOfFrequencies; ++m) {  // for every frequency m
                    sum = sum + (2*amplitude[j][m]*cos(2*PI_*frequency[m]*m_timeAtTimestep[t]-phi[j][m]));
				}
                tempVelocity[z][y][t].x = m_meanWindSpeedAtHeigth[z] + sum;  // store final result
                tempVelocity[z][y][t].y = 0;
                tempVelocity[z][y][t].z = 0;
				
				/* find the max and min value for the windfield */
                if (tempVelocity[z][y][t].x < m_minValueX) {
                    m_minValueX = tempVelocity[z][y][t].x;
				}
                if (tempVelocity[z][y][t].x > m_maxValueX) {
                    m_maxValueX = tempVelocity[z][y][t].x;
				}
				emit updateProgress();						
			}

			++y;
            if (y == m_pointsPerSideY) {
				y = 0;
				++z;
			}
		}
    }

    const float valueRangeX = m_maxValueX - m_minValueX;
    vslopeX = 65535 / valueRangeX;
    vinterceptX = -32768 - vslopeX*m_minValueX;

    m_maxValueY = 1;
    m_minValueY = -1;
    const float valueRangeY = m_maxValueY - m_minValueY;
    vslopeY = 65535 / valueRangeY;
    vinterceptY = -32768 - vslopeY*m_minValueY;

    m_maxValueZ = 1;
    m_minValueZ = -1;
    const float valueRangeZ = m_maxValueZ - m_minValueZ;
    vslopeZ = 65535 / valueRangeZ;
    vinterceptZ = -32768 - vslopeZ*m_minValueZ;

    // here we are converting the computed velocity values into the qint16 format to save memory
    if (! *m_cancelCalculation)
    {
        int z = 0;  // the row (or height) in the windfield
        int y = 0;  // the column in the windfield
        int t = 0;  // refers to the timesteps
        for (j = 0; j < pointsInTotal && ! *m_cancelCalculation; ++j) {  // for every point j
            for (t = 0; t < m_numberOfTimesteps; ++t) {  // for every timestep t

                m_resultantVelocity[z][y][t].x = tempVelocity[z][y][t].x*vslopeX+vinterceptX;  // store final result
                m_resultantVelocity[z][y][t].y = tempVelocity[z][y][t].y*vslopeY+vinterceptY;
                m_resultantVelocity[z][y][t].z = tempVelocity[z][y][t].z*vslopeZ+vinterceptZ;
            }

            ++y;
            if (y == m_pointsPerSideY) {
                y = 0;
                ++z;
            }
        }
    }
	
	
	/* * * * * * free thread shared memory * * * * * */
	delete [] frequency;
	delete [] psdNormalizationFactor;
	
	for (j = 0; j < pointsInTotal; ++j) {
		delete [] amplitude[j];
	}
	delete [] amplitude;
	
	for (j = 0; j < pointsInTotal; ++j) {
		delete [] phi[j];
	}
	delete [] phi;
	
	for (j = 0; j < pointsInTotal; ++j) {
		delete [] random[j];
	}
    delete [] random;

    for (int z = 0; z < m_pointsPerSideZ; ++z) {
        for (int y = 0; y < m_pointsPerSideY; ++y) {
            delete [] tempVelocity[z][y];
        }
        delete [] tempVelocity[z];
    }
    delete [] tempVelocity;

	/* * * * * * finish calculation * * * * * */
	if (! *m_cancelCalculation) {
		m_isValid = true;
		emit updateProgress();  // emits last update signal. Calculation is finished
	}

    if (m_isValid) PrepareGraphData();

}

void WindField::PrepareGraphData(){

    m_availableWindfieldVariables.clear();
    m_WindfieldGraphData.clear();

    m_availableWindfieldVariables.append("Time [s]");
    m_availableWindfieldVariables.append("Hub Height Velocity Abs [m/s]");
    m_availableWindfieldVariables.append("Hub Height X Velocity [m/s]");
    m_availableWindfieldVariables.append("Hub Height Y Velocity [m/s]");
    m_availableWindfieldVariables.append("Hub Height Z Velocity [m/s]");

    for (int i=0;i<m_probeLocations.size();i++){
        m_availableWindfieldVariables.append("Probe ("+QString().number(m_probeLocations.at(i).x,'f',1)+","+QString().number(m_probeLocations.at(i).y,'f',1)+","+QString().number(m_probeLocations.at(i).z,'f',1)+") Abs Velocity [m/s]");
        m_availableWindfieldVariables.append("Probe ("+QString().number(m_probeLocations.at(i).x,'f',1)+","+QString().number(m_probeLocations.at(i).y,'f',1)+","+QString().number(m_probeLocations.at(i).z,'f',1)+") X Velocity [m/s]");
        m_availableWindfieldVariables.append("Probe ("+QString().number(m_probeLocations.at(i).x,'f',1)+","+QString().number(m_probeLocations.at(i).y,'f',1)+","+QString().number(m_probeLocations.at(i).z,'f',1)+") Y Velocity [m/s]");
        m_availableWindfieldVariables.append("Probe ("+QString().number(m_probeLocations.at(i).x,'f',1)+","+QString().number(m_probeLocations.at(i).y,'f',1)+","+QString().number(m_probeLocations.at(i).z,'f',1)+") Z Velocity [m/s]");
    }

    QVector<float> time, ABShubHeightVelocity,XhubHeightVelocity,YhubHeightVelocity,ZhubHeightVelocity;

    for (int i=0;i<m_numberOfTimesteps;i++){
        time.append(i*m_assignedTimeStep);
        Vec3 velocity = getWindspeed(Vec3(0,0,m_hubheight),i*m_assignedTimeStep,false,false,0.0);
        ABShubHeightVelocity.append(velocity.VAbs());
        XhubHeightVelocity.append(velocity.x);
        YhubHeightVelocity.append(velocity.y);
        ZhubHeightVelocity.append(velocity.z);
    }

    m_WindfieldGraphData.append(time);
    m_WindfieldGraphData.append(ABShubHeightVelocity);
    m_WindfieldGraphData.append(XhubHeightVelocity);
    m_WindfieldGraphData.append(YhubHeightVelocity);
    m_WindfieldGraphData.append(ZhubHeightVelocity);

    for (int j=0;j<m_probeLocations.size();j++){
        QVector<float> abs,x,y,z;

        for (int i=0;i<m_numberOfTimesteps;i++){

            Vec3 velocity = getWindspeed(m_probeLocations.at(j),i*m_assignedTimeStep,false,false,0.0);
            abs.append(velocity.VAbs());
            x.append(velocity.x);
            y.append(velocity.y);
            z.append(velocity.z);
        }

        m_WindfieldGraphData.append(abs);
        m_WindfieldGraphData.append(x);
        m_WindfieldGraphData.append(y);
        m_WindfieldGraphData.append(z);
    }
}

void WindField::AddProbe(Vec3 location){

    bool found = false;
    for (int i=0;i<m_probeLocations.size();i++)
        if (m_probeLocations[i] == location) found = true;

    if (!found) m_probeLocations.append(location);

    PrepareGraphData();


}

void WindField::DeleteProbe(int index){

    if (index < m_probeLocations.size()) m_probeLocations.removeAt(index);

    PrepareGraphData();

}

Vec3 WindField::getWindspeed(Vec3 vec, double time, bool mirror, bool isAutoFielShift, double shiftTime){
    double z,y;
    z = vec.z;
    y = vec.y;

    z -= m_bottomZ;
    y += m_fieldDimensionY/2;

    //here the windfield is marched trhough the domain with the hub height wind speed. initially it is marched through the domain for half a windfield diameter

    if (isAutoFielShift) time += (m_fieldDimensionY/2.0 - vec.x) / m_meanWindSpeedAtHub;
    else time += shiftTime;

    if (time < 0) time = 0;

    //mirror windfield, windfields are mirrored at the ends and then pieced together

    if (mirror){
        time = fabs(time);

        while (time > 2.0*m_simulationTime)
            time -= 2.0*m_simulationTime;


        if (time > m_simulationTime)
            time = 2.0*m_simulationTime - time;

    }
    else{
        while (time > m_simulationTime)
            time -= m_simulationTime;

    }

    //z is constant above and below the field dimensions
    if ( z > m_fieldDimensionZ) z = m_fieldDimensionZ;
    if ( z < 0) z = 0;

    //y is mirrored at the bounds
    y = fabs(y);

    while (y > 2.0*m_fieldDimensionY){
        y -= 2.0*m_fieldDimensionY;
    }

    if (y > m_fieldDimensionY){
        y = 2.0*m_fieldDimensionY - y;
    }


    {

        // start interpolation
        double spatialwidthY = m_fieldDimensionY / (m_pointsPerSideY-1);
        double spatialwidthZ = m_fieldDimensionZ / (m_pointsPerSideZ-1);
        double temporalwidth = m_simulationTime / (m_numberOfTimesteps-1);

        int zindex = floor(z / spatialwidthZ);
        int yindex = floor(y / spatialwidthY);
        int tindex = floor(time / temporalwidth);
        
        if (tindex > (m_numberOfTimesteps-2)){
            time = m_simulationTime;
            tindex = (m_numberOfTimesteps-2);
        }

        if (zindex > (m_pointsPerSideZ-2)){
            zindex = (m_pointsPerSideZ-2);
        }

        if (yindex > (m_pointsPerSideY-2)){
            yindex = (m_pointsPerSideY-2);
        }
        
        Vec3f mZYfloorTfloor, mZYceiltTfloor, mZYfloorTceil, mZYceiltTceil;
        
        mZYfloorTfloor.x = convertX(m_resultantVelocity[zindex][yindex][tindex].x)+(convertX(m_resultantVelocity[zindex+1][yindex][tindex].x)-convertX(m_resultantVelocity[zindex][yindex][tindex].x))*(z-spatialwidthZ*zindex)/spatialwidthZ;
        mZYfloorTfloor.y = convertY(m_resultantVelocity[zindex][yindex][tindex].y)+(convertY(m_resultantVelocity[zindex+1][yindex][tindex].y)-convertY(m_resultantVelocity[zindex][yindex][tindex].y))*(z-spatialwidthZ*zindex)/spatialwidthZ;
        mZYfloorTfloor.z = convertZ(m_resultantVelocity[zindex][yindex][tindex].z)+(convertZ(m_resultantVelocity[zindex+1][yindex][tindex].z)-convertZ(m_resultantVelocity[zindex][yindex][tindex].z))*(z-spatialwidthZ*zindex)/spatialwidthZ;
        
        mZYceiltTfloor.x = convertX(m_resultantVelocity[zindex][yindex+1][tindex].x)+(convertX(m_resultantVelocity[zindex+1][yindex+1][tindex].x)-convertX(m_resultantVelocity[zindex][yindex+1][tindex].x))*(z-spatialwidthZ*zindex)/spatialwidthZ;
        mZYceiltTfloor.y = convertY(m_resultantVelocity[zindex][yindex+1][tindex].y)+(convertY(m_resultantVelocity[zindex+1][yindex+1][tindex].y)-convertY(m_resultantVelocity[zindex][yindex+1][tindex].y))*(z-spatialwidthZ*zindex)/spatialwidthZ;
        mZYceiltTfloor.z = convertZ(m_resultantVelocity[zindex][yindex+1][tindex].z)+(convertZ(m_resultantVelocity[zindex+1][yindex+1][tindex].z)-convertZ(m_resultantVelocity[zindex][yindex+1][tindex].z))*(z-spatialwidthZ*zindex)/spatialwidthZ;
        
        Vec3f meanTfloor = mZYfloorTfloor+Vec3f(mZYceiltTfloor-mZYfloorTfloor)*(y-spatialwidthY*yindex)/spatialwidthY;
        
        mZYfloorTceil.x = convertX(m_resultantVelocity[zindex][yindex][tindex+1].x)+(convertX(m_resultantVelocity[zindex+1][yindex][tindex+1].x)-convertX(m_resultantVelocity[zindex][yindex][tindex+1].x))*(z-spatialwidthZ*zindex)/spatialwidthZ;
        mZYfloorTceil.y = convertY(m_resultantVelocity[zindex][yindex][tindex+1].y)+(convertY(m_resultantVelocity[zindex+1][yindex][tindex+1].y)-convertY(m_resultantVelocity[zindex][yindex][tindex+1].y))*(z-spatialwidthZ*zindex)/spatialwidthZ;
        mZYfloorTceil.z = convertZ(m_resultantVelocity[zindex][yindex][tindex+1].z)+(convertZ(m_resultantVelocity[zindex+1][yindex][tindex+1].z)-convertZ(m_resultantVelocity[zindex][yindex][tindex+1].z))*(z-spatialwidthZ*zindex)/spatialwidthZ;
        
        mZYceiltTceil.x = convertX(m_resultantVelocity[zindex][yindex+1][tindex+1].x)+(convertX(m_resultantVelocity[zindex+1][yindex+1][tindex+1].x)-convertX(m_resultantVelocity[zindex][yindex+1][tindex+1].x))*(z-spatialwidthZ*zindex)/spatialwidthZ;
        mZYceiltTceil.y = convertY(m_resultantVelocity[zindex][yindex+1][tindex+1].y)+(convertY(m_resultantVelocity[zindex+1][yindex+1][tindex+1].y)-convertY(m_resultantVelocity[zindex][yindex+1][tindex+1].y))*(z-spatialwidthZ*zindex)/spatialwidthZ;
        mZYceiltTceil.z = convertZ(m_resultantVelocity[zindex][yindex+1][tindex+1].z)+(convertZ(m_resultantVelocity[zindex+1][yindex+1][tindex+1].z)-convertZ(m_resultantVelocity[zindex][yindex+1][tindex+1].z))*(z-spatialwidthZ*zindex)/spatialwidthZ;
        
        Vec3f meanTceil = mZYfloorTceil+Vec3f(mZYceiltTceil-mZYfloorTceil)*(y-spatialwidthY*yindex)/spatialwidthY;
        
        
        Vec3f Vint = meanTfloor+Vec3f(meanTceil-meanTfloor)*(time-temporalwidth*tindex)/temporalwidth;
        
        return Vec3(Vint.x,Vint.y,Vint.z);
    }
}

float WindField::getDist (const float y1, const float z1, const float y2, const float z2) {
	const float dy = (y1-y2);
	const float dz = (z1-z2);
	
	if (dy != 0 && dz != 0) {
		return sqrt(dy*dy + dz*dz);
	} else if (dz != 0) {  // therefore dy must be 0
		return dz;
	} else if (dy != 0) {
		return dy;
	} else {
		return 0;
	}
}

float WindField::getCoh(const float frequency, const float spatialDistance) {
	/* Coherence function according to Frost.
	 * The coherence decrement C is set to 12 as Hansen suggested.
	 * */
	return exp(-12 * frequency * spatialDistance / m_meanWindSpeed);  // meanWindSpeed not depending on height?
}

float WindField::getPSD (const float frequency, const int zIndexOfPoint) {
    const float heigthAboveGround = m_hubheight + m_zCoordinates[zIndexOfPoint];
	float l;
	if (heigthAboveGround <= 30) {
		l = heigthAboveGround*20;
	} else {
		l = 600;
	}
	
	/* PSD function after Kaimal as in Hansen p. 149 */
	const float turbulenceDecimal = m_turbulenceIntensity / 100;
	return ( turbulenceDecimal*turbulenceDecimal * m_meanWindSpeedAtHeigth[zIndexOfPoint] * l ) /
			( pow((1 + frequency * 1.5 * l / m_meanWindSpeedAtHeigth[zIndexOfPoint]), (5.0/3)) );
}

QVariant WindField::accessParameter(Parameter::Windfield::Key key, QVariant value) {
	typedef Parameter::Windfield P;
	
	const bool set = value.isValid();
	switch (key) {
	case P::Name: if(set) m_objectName = value.toString(); else value = m_objectName; break;
    case P::Time: if(set) m_assignedSimulationTime = value.toFloat(); else value = m_assignedSimulationTime; break;
    case P::TimestepSize: if(set) m_assignedTimeStep = value.toFloat(); else value = m_assignedTimeStep; break;
    case P::Points: if(set) m_pointsPerSideY = value.toInt(); else value = m_pointsPerSideY; break;
    case P::FieldWidth: if(set) m_fieldRadius = value.toFloat()/2.0; else value = m_fieldRadius*2.0; break;
	case P::HubHeight: if(set) m_hubheight = value.toFloat(); else value = m_hubheight; break;
	case P::WindSpeed: if(set) m_meanWindSpeed = value.toFloat(); else value = m_meanWindSpeed; break;
	case P::Turbulence: if(set) m_turbulenceIntensity = value.toFloat(); else value = m_turbulenceIntensity; break;
    case P::ShearLayer: if(set) m_profileModel = value.toInt(); else value = m_profileModel; break;
	case P::MeasurementHeight: if(set) m_windSpeedMeasurementHeight = value.toFloat();
							   else value = m_windSpeedMeasurementHeight; break;
	case P::RoughnessLength: if(set) m_roughnessLength = value.toFloat(); else value = m_roughnessLength; break;
    case P::Seed: if(set) m_seed = value.toUInt(); else value = m_seed; break;
    case P::ShearExponent: if(set) m_shearExponent = value.toDouble(); else value = m_shearExponent; break;

	}

	return (set ? QVariant() : value);
}

WindField* WindField::newBySerialize() {
	WindField* windfield = new WindField ();
	windfield->serialize();

    if (windfield->m_isValid) windfield->PrepareGraphData();

	return windfield;
}

WindField *WindField::newByImport(QDataStream &dataStream) {
	WindField* windfield = new WindField;
	windfield->importFromBinary(dataStream);

    if (windfield->m_isValid) windfield->PrepareGraphData();

	return windfield;
}
