#ifndef BUNNY_STRAND_DROP_MANY_H
#define BUNNY_STRAND_DROP_MANY_H

#include "SIMULATION_SCENE.h"
#include "Timestepper/Volume/BDF_1.h"
#include "Timestepper/Volume/BDF_2.h"
#include "Timestepper/Strand_Volume/TIMESTEPPER.h"
#include "Geometry/STRAND_MESH_FASTER.h"
#include "Geometry/STRAND_MESH.h"
#include "Geometry/BOWL.h"
#include "Hyperelastic/Strand/QUADRATIC_STRETCHING.h"
#include "Hyperelastic/Strand/BERGOU_2010.h"
#include "Hyperelastic/Strand/ISOTROPIC_BENDING.h"
#include "Hyperelastic/Strand/QUADRATIC_BENDING.h"
#include "Hyperelastic/Strand/QUADRATIC_UNIT_BENDING.h"
#include "Hyperelastic/Strand/ISOTROPIC_THETA.h"

namespace HOBAK {

class BUNNY_STRAND_DROP_MANY : public SIMULATION_SCENE {
public:

  virtual void printSceneDescription() override
  {
    cout << "=====================================================================" << endl;
    cout << " Dropping a bunny down an obstacle course to test out both kinematic" << endl;
    cout << " and self-collisions. Both VF and EE collisions are enabled." << endl;
    cout << "=====================================================================" << endl;
  }

  virtual bool buildScene() override
  {
    // this will determine the MOV and JSON filenames
    _sceneName = "bunny_strand_drop";

    // read in the tet mesh file
    //_tetMeshFilename = string("../data/scorpion_0_125.tobj");
    _tetMeshFilename = string("../data/bunny/bunny_5.tobj");
    _strandMeshFilename = "../data/singleCurl.sobj";
    //_tetMeshFilename = string("../data/Brachiosaurus_15.tobj");
    //_tetMeshFilename = string("../data/dragon.tobj");

    // read tet
    vector<VECTOR3> vertices;
    vector<VECTOR4I> tets;
    bool success = TET_MESH::readTobjFile(_tetMeshFilename, vertices, tets);
    if (!success)
    {
      cout << " Failed to open file " << _tetMeshFilename.c_str() << endl;
      return false;
    }
    vertices = TET_MESH::normalizeVertices(vertices);
    _normalizedVertices = true;


    using namespace Eigen;
    // _E = 1e8;
    // _G = 1e8;
    // _spacing = 1.0;
    // _density = 1.5;
    // _baseRadius = 0.005;
    // _tipRadius = 0.005;
    // _drawVertices = true;
    // _totalStrands = 1;

    _E = 3.9e6;
    _G = 3.9e6;
    _spacing = 1.0;
    _density = 1.3;
    _baseRadius = 0.1;
    _tipRadius = 0.1;
    _drawVertices = true;
    _totalStrands = 1;

    // read strand from file
    // vector<vector<int> > strandIndices;
    // std::vector<VECTOR3> restVerticesStrand;
    // if (!readSOBJFile(_strandMeshFilename, restVerticesStrand, strandIndices)) { 
    //   cout << " Failed to open file " << _strandMeshFilename << endl;
    //   std::exit(1); }
    // // _initialAStrand = 0.15 * MATRIX3::Identity() * AngleAxisd(0.5 * M_PI, VECTOR3::UnitZ());
    // _initialAStrand = MATRIX3::Identity() * AngleAxisd(0.5 * M_PI, VECTOR3::UnitZ());
    // // _initialAStrand = MATRIX3::Identity();
    // _initialTranslationStrand = VECTOR3(2.0, -4.0, 0.0);
    // // _initialTranslationStrand = VECTOR3(2.0, -1.0, 0.0);
    // for (unsigned int x = 0; x < restVerticesStrand.size(); x++)
    //   restVerticesStrand[x] = _initialAStrand * restVerticesStrand[x] + _initialTranslationStrand;
    // _strandMesh = new TET_WISP_MESH(restVerticesStrand, strandIndices,
    //                       _E, _G, _density, _baseRadius, _tipRadius);

    // construct hair tie
    // apply transformation
    _initialAStrand = MATRIX3::Identity();
    // _initialAStrand = MATRIX3::Identity();
    _initialTranslationStrand = VECTOR3(-2.5, 5.0, 0.0);
    // _initialTranslationStrand = VECTOR3(2.0, -1.0, 0.0);
    buildHairTie();

    _strandMesh -> setCollisionEps(0.1);
    _strandMesh->bendingForceFilterEnabled() = true;

    const REAL k = 1.0;
    _stretchingEnergy = new STRAND::QUADRATIC_STRETCHING(k);

    MATRIX3 M;
    // M =   AngleAxisd(-0.5 * M_PI, VECTOR3::UnitX())
    // //M =   AngleAxisd(0 , VECTOR3::UnitX())
    //     * AngleAxisd(0,  VECTOR3::UnitY())
    //     * AngleAxisd(0, VECTOR3::UnitZ());

    REAL bunnyScale = 5.0;

    M =  bunnyScale * MATRIX3::Identity() * AngleAxisd(-0.5 * M_PI, VECTOR3::UnitX())
    //M =   AngleAxisd(0 , VECTOR3::UnitX())
        * AngleAxisd(0,  VECTOR3::UnitY())
        * AngleAxisd(0, VECTOR3::UnitZ());
    VECTOR3 half(0.5, 0.5, 0.5);
   
    _initialA           = M;
    _initialTranslation = half - M * half + VECTOR3(-1.5,-5.0,1.5);
    // _initialTranslation = half - M * half;

    for (unsigned int x = 0; x < vertices.size(); x++)
      vertices[x] = _initialA * vertices[x] + _initialTranslation;

    _gravity = VECTOR3(0, -1.0, 0);

    // REAL E = 3.0;
    // //REAL nu = 0.3; // lambda \approx 10
    // REAL nu = 0.45; // lambda \approx 10
    REAL E = 100.0;
    REAL nu = 0.45; 

    REAL mu     = VOLUME::HYPERELASTIC::computeMu(E, nu);
    REAL lambda = VOLUME::HYPERELASTIC::computeLambda(E, nu);
    cout << " mu:     " << mu << endl;
    cout << " lambda: " << lambda << endl;

    // get more bunnies
    const int numOtherBunnies = 2;
    vector<VECTOR3> otherBunnyTranslations(numOtherBunnies);
    const int bunnyVertices = (int)vertices.size();
    const int bunnyTets = (int)tets.size();
    otherBunnyTranslations[0] = VECTOR3(-bunnyScale, 0.0, 0.0);
    otherBunnyTranslations[1] = VECTOR3(bunnyScale, 0.0, 0.0);
    for(unsigned int y = 0; y < numOtherBunnies; y++) {
      for(unsigned int x = 0; x < bunnyVertices; x++) {
        vertices.push_back(vertices[x] + otherBunnyTranslations[y]);
      }
      for(unsigned int z = 0; z < bunnyTets; z++) {
        tets.push_back(tets[z] + VECTOR4I(bunnyVertices * y, bunnyVertices * y,
                                          bunnyVertices * y, bunnyVertices * y));
      }
    }

    // build the tet mesh object
    _tetMesh = new TET_MESH_FASTER(vertices, tets);
    _hyperelastic = new VOLUME::SNH(mu, lambda);
    //_hyperelastic = new VOLUME::ARAP(mu * 4, lambda);

    const vector<REAL>& areas = _tetMesh->surfaceTriangleAreas();
    REAL smallest = areas[0];
    REAL largest = areas[0];
    for (unsigned int x = 1; x < areas.size(); x++)
    {
      if (areas[x] > largest) largest  = areas[x];
      if (areas[x] < largest) smallest = areas[x];
    }
    cout << "Largest triangle area:  "  << largest << endl;
    cout << "Smallest triangle area: "  << smallest << endl;

    // build the time integrator
    // _strandVolumeSolver = new VOLUME::BACKWARD_EULER_POSITION(*_tetMesh, *_hyperelastic);
    _strandVolumeSolver = new STRAND_VOLUME::TIMESTEPPER(*_strandMesh, *_stretchingEnergy, *_tetMesh, *_hyperelastic);

    const int maxNewtonIterations = 3;  // works with warm start
    _strandVolumeSolver->maxNewtonIterations() = maxNewtonIterations;
    //_strandVolumeSolver = new VOLUME::BDF_2(*_tetMesh, *_hyperelastic);
    //
    // FLT_EPSILON in VOLUME::TIMESTEPPER::findNewSurfaceConstraints does not play nice here;
    // it seems to like it when it's set to zero. Needs further investigation.
    // Plus the conditioning of the matrix seems much worse than position-based
    //_strandVolumeSolver = new VOLUME::BACKWARD_EULER_VELOCITY(*_tetMesh, *_hyperelastic);
    _strandVolumeSolver->setDt(1.0 / 30.0);

    _kinematicShapes.reserve(10);
    vector<VECTOR3> centers;
    centers.reserve(10);

    // VECTOR3 center(0.0, -10, 0.0);
    // centers.push_back(center);
    // _kinematicShapes.push_back(new CUBE(centers.back(), 10.0));
    // _strandVolumeSolver->addKinematicCollisionObject(_kinematicShapes.back());
    
    // ground
    // VECTOR3 center(0.0, -35, 0.0);
    // centers.push_back(center);
    // _kinematicShapes.push_back(new CUBE(centers.back(), 40.0));
    // _strandVolumeSolver->addKinematicCollisionObject(_kinematicShapes.back());

    // bowl
    VECTOR3 center(0.0, -5.0, 0.0);
    centers.push_back(center);
    _kinematicShapes.push_back(new BOWL(centers.back(), 0.9,  10.0));
    _strandVolumeSolver->addKinematicCollisionObject(_kinematicShapes.back());

    // strand box
    center = VECTOR3(1.5, 8.0, 0.0);
    centers.push_back(center);
    _kinematicShapes.push_back(new CUBE(centers.back(), 14.0));
    _strandVolumeSolver->attachKinematicSurfaceConstraints(_kinematicShapes.back(), true);
    // _strandVolumeSolver->addKinematicCollisionObject(_kinematicShapes.back());

    // top container                                
    // center = VECTOR3(0.0, 25, 0.0);
    // centers.push_back(center);
    // _kinematicShapes.push_back(new CUBE(centers.back(), 40.0));
    // _strandVolumeSolver->attachKinematicSurfaceConstraints(_kinematicShapes.back(), true);
    // _strandVolumeSolver->addKinematicCollisionObject(_kinematicShapes.back());
    
    // center = VECTOR3(0.25, 0.0, 0.25);
    // centers.push_back(center);
    // _kinematicShapes.push_back(new CUBE(centers.back(), 1.0));
    // _kinematicShapes.back()->rotation() = Eigen::AngleAxisd(M_PI * 0.25, VECTOR3::UnitZ());
    // _strandVolumeSolver->addKinematicCollisionObject(_kinematicShapes.back());

    // center = VECTOR3(2.0, -0.75, 0.25);
    // centers.push_back(center);
    // _kinematicShapes.push_back(new CUBE(centers.back(), 1.0));
    // _kinematicShapes.back()->rotation() = Eigen::AngleAxisd(M_PI * 0.25, VECTOR3::UnitZ());
    // _strandVolumeSolver->addKinematicCollisionObject(_kinematicShapes.back());

    // center = VECTOR3(0.25, -1.5, 0.25);
    // centers.push_back(center);
    // _kinematicShapes.push_back(new CUBE(centers.back(), 1.0));
    // _kinematicShapes.back()->rotation() = Eigen::AngleAxisd(M_PI * 0.25, VECTOR3::UnitZ());
    // _strandVolumeSolver->addKinematicCollisionObject(_kinematicShapes.back());

    // center = VECTOR3(2.0, -2.25, 0.25);
    // centers.push_back(center);
    // _kinematicShapes.push_back(new CUBE(centers.back(), 1.0));
    // _kinematicShapes.back()->rotation() = Eigen::AngleAxisd(M_PI * 0.25, VECTOR3::UnitZ());
    // _strandVolumeSolver->addKinematicCollisionObject(_kinematicShapes.back());

    // center = VECTOR3(0.25, -3.0, 0.25);
    // centers.push_back(center);
    // _kinematicShapes.push_back(new CUBE(centers.back(), 1.0));
    // _kinematicShapes.back()->rotation() = Eigen::AngleAxisd(M_PI * 0.25, VECTOR3::UnitZ());
    // _strandVolumeSolver->addKinematicCollisionObject(_kinematicShapes.back());

    // center = VECTOR3(2.0, -3.75, 0.25);
    // centers.push_back(center);
    // _kinematicShapes.push_back(new CUBE(centers.back(), 1.0));
    // _kinematicShapes.back()->rotation() = Eigen::AngleAxisd(M_PI * 0.25, VECTOR3::UnitZ());
    // _strandVolumeSolver->addKinematicCollisionObject(_kinematicShapes.back());

    // center = VECTOR3(0.25, -4.5, 0.25);
    // centers.push_back(center);
    // _kinematicShapes.push_back(new CUBE(centers.back(), 1.0));
    // _kinematicShapes.back()->rotation() = Eigen::AngleAxisd(M_PI * 0.25, VECTOR3::UnitZ());
    // _strandVolumeSolver->addKinematicCollisionObject(_kinematicShapes.back());

    // center = VECTOR3(2.0, -5.25, 0.25);
    // centers.push_back(center);
    // _kinematicShapes.push_back(new CUBE(centers.back(), 1.0));
    // _kinematicShapes.back()->rotation() = Eigen::AngleAxisd(M_PI * 0.25, VECTOR3::UnitZ());
    // _strandVolumeSolver->addKinematicCollisionObject(_kinematicShapes.back());
    // collision constants
    const REAL collisionMu = 1000.0;
    _strandVolumeSolver->collisionsEnabled() = true;
    _strandVolumeSolver->pcgEnabled() = false;
    _strandVolumeSolver->hessianClampingEnabled() = true;
    _strandVolumeSolver->collisionStiffnessVolume() = collisionMu;
    _strandVolumeSolver->collisionDampingBetaVolume() = 0.01;
    _strandVolumeSolver->collisionStiffnessStrand() = 1e4;
    _strandVolumeSolver->collisionDampingBetaStrand() = 0.0;
    _strandVolumeSolver->vertexFaceSelfCollisionsOn() = true;
    _strandVolumeSolver->edgeEdgeSelfCollisionsOn() = true;

    _strandMesh->bendingForceFilterEnabled() = false;

    // _eye    = VECTOR3(1.7, -2.25, 8.5);
    // _lookAt = VECTOR3(1.6, -2.25, 7.5);
    // _up     = VECTOR3(0.0, 1.0, 0.0);

    _eye    = VECTOR3(1.7, -2.25, 40.5);
    _lookAt = VECTOR3(1.6, -2.25, 7.5);
    _up     = VECTOR3(0.0, 1.0, 0.0);

    _worldCenter = VECTOR3( 0.497, 0.785889, 0.452556);
    _pauseFrame = 600;
    // _pauseFrame = 400;
    return true;
  }

  void buildHairTie() 
  {
    cout<<"building hair tie."<<endl;
    vector<VECTOR3> restVertices;
    vector<vector<int> > strands;

    // const REAL amplitude = 0.5;
    _spacing = 8.0;
    const REAL radiusCurl = 0.50;
    const REAL radiusRing = 2.0;
    _totalPoints = 100;
    int frequency = 5;
    REAL ringStep = 2 * M_PI / _totalPoints;
    REAL curlStep = 2 * M_PI / frequency;
    REAL thetaRing = 0.0;
    REAL thetaCurl = 0.0;
    int idxCurl = 0;
    vector<int> strand0;
    vector<VECTOR3> centerPositions(4);
    centerPositions[0] = VECTOR3(0.0, 0.0, 0.0);
    centerPositions[1] = VECTOR3(_spacing, 0.0, 0.0);
    centerPositions[2] = VECTOR3(0.0, _spacing, 0.0);
    centerPositions[3] = VECTOR3(_spacing, _spacing, 0.0);
    for(unsigned c = 0; c < 4; c++){
      strand0.clear();
      for(unsigned int i = 0; i < _totalPoints; i++) 
      {
        VECTOR3& centerPosition = centerPositions[c];
        VECTOR3 ringPosition(radiusRing * cos(thetaRing), 0.0, - radiusRing * sin(thetaRing));
        VECTOR3 curlBasis0 = ringPosition.normalized() * radiusCurl;
        VECTOR3 curlBasis1(0.0, radiusCurl, 0.0);
        VECTOR3 curlPosition = cos(thetaCurl) * curlBasis0 + sin(thetaCurl) * curlBasis1;
        restVertices.push_back(ringPosition + curlPosition + centerPosition);
        strand0.push_back(i + c * _totalPoints);
        thetaRing += ringStep;
        idxCurl = (idxCurl + 1) % frequency;
        thetaCurl = -idxCurl * curlStep;
      }
      strand0.push_back(c * _totalPoints);
      strands.push_back(strand0);
    }

    for (unsigned int x = 0; x < restVertices.size(); x++)
      restVertices[x] = _initialAStrand * restVertices[x] + _initialTranslationStrand;
    
    _strandMesh = new TET_WISP_MESH(restVertices, strands,
                                    _E, _G, _density, _baseRadius, _tipRadius);
    cout<<"done."<<endl;
  }

  virtual void stepSimulation(const bool verbose = true) override
  {
    _strandVolumeSolver->externalForces().setZero();
    _strandVolumeSolver->addGravity(_gravity);
    _strandVolumeSolver->solveDynamics(verbose);
    // _strandVolumeSolver->solveNewton(verbose);

    if (_writeToFile) {
      if (!writeFrameToFile()) {
        // Do something here that isn't too noisy.
      }
    }

    if(_frameNumber == 100)
      _strandVolumeSolver->removeKinematicSurfaceConstraints();

    _frameNumber++;
  }
#ifndef GL_DISABLED
  virtual void drawScene()
  {
    //drawAxes();
    drawStrandMesh(*_strandMesh);
    drawSurfaceTriangles(*_tetMesh, true);
    //drawStrandMeshOld(*_strandMesh);

    glEnable(GL_DEPTH_TEST);
    for (unsigned int x = 0; x < _kinematicShapes.size(); x++)
      drawKinematicShape(*_kinematicShapes[x]);

    //drawCollisionsOld(*_strandMesh);
    // drawCollisions(*_strandMesh);

    /*
    drawKinematicConstraints(_strandMesh, _strandSolver);
    //drawPlaneConstraints(_triangleMesh, _strandSolver);
    drawStrandTwistFreeFrames(*_strandMesh);
    drawStrandTwistFrames(*_strandMesh);
    drawStrandTwistForces(*_strandMesh);
    */
  };
#endif
protected:
  // strand generation parameters
  char* _strandMeshFilename;
  STRAND_MESH* _strandMesh;
  STRAND::STRETCHING* _stretchingEnergy;
  unsigned int _totalPoints;
  unsigned int _totalStrands;
  REAL _spacing;
  REAL _E;
  REAL _nu;
  REAL _density;
  REAL _baseRadius;
  REAL _tipRadius;
  STRAND_VOLUME::TIMESTEPPER* _strandVolumeSolver;

  // GL drawing params
  bool _drawVertices;

  // initial rotation-scale and translation of tet mesh
  MATRIX3 _initialAStrand;
  VECTOR3 _initialTranslationStrand;
};

}

#endif