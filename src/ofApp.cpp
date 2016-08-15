#include "ofApp.h"

Particle::Particle(float pos_x, float pos_y, float vel_x, float vel_y) :
	position (pos_x, pos_y),
	velocity (vel_x, vel_y),
	acceleration (0.0, 0.0),
	add_radius (0.0),
	friction (0.0005),
  volumeAve (0.0),
  erapsedTime (0)
{

}

void Particle::addRadius(float r)
{
	add_radius = r;
}

void Particle::resetAcceleration()
{
	acceleration.set(0.0, 0.0);
}

void Particle::addAcceleration(float acc_x, float acc_y)
{
	acceleration.set(acc_x, acc_y);
}

void Particle::addFriction(){
	acceleration.x -= velocity.x * friction;
	acceleration.y -= velocity.y * friction;
}

void Particle::update(float volAve){
	velocity += acceleration;
	position += velocity;
  volumeAve = volAve;
  ++erapsedTime;
}

void Particle::draw(){
	ofDrawCircle(position.x, position.y, 3.0 + add_radius + 100.0 * volumeAve);
}

//--------------------------------------------------------------
ofApp::ofApp() :
 cursor_draw (false),
 senderSetup (false),
 wheelValue (0.0),
 hueValue (0.0),
 spread_mul (100.0)
{

}

//--------------------------------------------------------------
void ofApp::setup(){
  //oF Core Settings
  ofSetLogLevel(OF_LOG_VERBOSE);
  //Display Settings
	ofSetWindowTitle("Mind Tracker");
	ofSetVerticalSync(true);
	ofSetFrameRate(DRAW_FPS);
	ofSetBackgroundAuto(false);
	ofBackground(0,0,0);
  //Input Sound Device Settings
	soundStream.printDeviceList();
	soundStream.setup(this, 0, 2, 48000, 32, 4);
  //Memory Alloc
  calc_mesh = new double**[TIME];
  unsigned int i = 0;
  while(i<TIME){
    calc_mesh[i] = new double*[COLUMN];
    unsigned int j = 0;
    while(j<COLUMN){
      calc_mesh[i][j] = new double[ROW];
      unsigned int k = 0;
      while(k<ROW){
          calc_mesh[i][j][k] = 0.0;
          ++k;
      }
      ++j;
    }
    ++i;
  }
	//OSC TeleCommunication Preparation
	oscReceiver.setup(PORT);
	cout << "listening for osc messages on port " << PORT << endl;
	oscSender.setup(HOST, PORT);
	senderSetup = true;
	cout << "OSC Messages will be sended to " << HOST << ":" << PORT << endl;
}

//--------------------------------------------------------------
void ofApp::update(){
	ofxOscMessage m;
	while(oscReceiver.hasWaitingMessages()){
		oscReceiver.getNextMessage(m);
		if(m.getAddress() == "/mouse/position"){
			particles.push_back(Particle(m.getArgAsInt32(0), m.getArgAsInt32(1), 0.2 * m.getArgAsInt32(2), 0.2 * m.getArgAsInt32(3)));
			particles.push_back(Particle(m.getArgAsInt32(0), m.getArgAsInt32(1), ofRandom(-0.01 * m.getArgAsInt32(3), 0.01 * m.getArgAsInt32(3)), ofRandom(-0.01 * m.getArgAsInt32(2), 0.01 * m.getArgAsInt32(2))));
		}else if(m.getAddress() == "/mouse/wheelvalue"){
			wheelValue = (wheelValue + m.getArgAsInt32(0)) / 2;
		}else if(m.getAddress() == "/mic/volume"){
			volumeIn = (volumeIn + m.getArgAsFloat(0)) / 2.0;
			volumeAve = (volumeAve + m.getArgAsFloat(1)) / 2.0;
		}else if(m.getAddress() == "/calcmesh/currentvalue"){
			calc_mesh[1][m.getArgAsInt32(0)][m.getArgAsInt32(1)] += m.getArgAsDouble(2);
		}
	}
  //Calculate Wave Equation
  unsigned int i = 1;
  while(i<COLUMN-1){
    unsigned int j = 1;
    while(j<ROW-1){
      calc_mesh[0][i][j] = 2 * calc_mesh[1][i][j] - calc_mesh[2][i][j] + 0.1 * (calc_mesh[1][i+1][j] + calc_mesh[1][i-1][j] + calc_mesh[1][i][j+1] + calc_mesh[1][i][j-1] - 4 * calc_mesh[1][i][j]);
      ++j;
    }
    ++i;
  }
  //Apply Diffusion Equation
  i = 1;
  while(i<COLUMN-1){
    unsigned int j = 1;
    while(j<ROW-1){
      calc_mesh[1][i][j] = calc_mesh[0][i][j];
      calc_mesh[2][i][j] = calc_mesh[1][i][j];
      ++j;
    }
    ++i;
  }
  i = 0;
  while(i<particles.size()){
    particles[i].update(volumeAve);
		particles[i].addFriction();
		particles[i].addAcceleration(ofRandom(-volumeIn, volumeIn), ofRandom(-volumeIn, volumeIn));
    ++i;
  }
  i = 0;
  while(i<particles.size()){
    if(particles[i].erapsedTime >= 70 || particles[i].position.x < 0.0 || particles[i].position.x > ofGetWidth() || particles[i].position.y < 0.0 || particles[i].position.y > ofGetHeight()){
      particles.erase(particles.begin() + i);
    }
    ++i;
  }
	float randomX = ofGetWidth() / 2 + ofRandom(0, ofMap(volumeAve, -1, 1, -ofGetWidth() / 2, ofGetWidth() / 2)), randomY = ofGetHeight() / 2 + ofRandom(0, ofMap(volumeAve, -1, 1, -ofGetHeight() / 2, ofGetHeight() / 2));
	i = randomX * ROW / (double)ofGetWidth() + 1;
	unsigned int j = randomY * COLUMN / (double)ofGetHeight() + 1;
  if(i <= 1 || i >= ROW-1 || j <= 1 || j >= COLUMN-1){
    return;
  }
	double meshAddValue = 20.0 * volumeAve;
	particles.push_back(Particle(randomX, randomY, ofRandom(0.0, volumeIn), ofRandom(0.0, volumeIn)));
	calc_mesh[1][i][j] += meshAddValue;
}

//--------------------------------------------------------------
void ofApp::draw(){
	hueValue = wheelValue + 10.0 * volumeIn;
	if(hueValue < 0.0){
		hueValue = 0.0;
	}else if(hueValue > 50.0){
		hueValue = 50.0;
	}
	if(cursor_draw){
		ofNoFill();
  	ofColor c;
  	float hueValue_volumeIn = hueValue + 10.0 * volumeIn;
  	if(hueValue_volumeIn < 0.0){
    	hueValue_volumeIn = 0.0;
  	}else if(hueValue > 50.0){
    	hueValue_volumeIn = 50.0;
  	}
  	c.setHsb(ofMap(hueValue_volumeIn, 50, 0, 0, 255), 255, 255, 255);
  	ofSetColor(c);
		ofEnableBlendMode(OF_BLENDMODE_ADD);
		ofSetLineWidth(0.02);
  	ofDrawBezier(ofGetWidth() / 2, ofGetHeight() / 2, ofGetWidth() / 2 + (mouseX - ofGetWidth() / 2) / 3 + ofRandom(-volumeIn, volumeIn), ofGetHeight() / 2 + (mouseY - ofGetHeight() / 2) / 3 + ofRandom(-volumeIn, volumeIn), ofGetWidth() / 2 + (mouseX - ofGetWidth() / 2) * 2 / 3 + ofRandom(-volumeIn, volumeIn), ofGetHeight() / 2 + (mouseY - ofGetHeight() / 2) * 2 / 3 + ofRandom(-volumeIn, volumeIn), mouseX, mouseY);
		c.setHsb(ofMap(hueValue_volumeIn, 0, 50, 0, 255), ofMap(volumeIn, -1, 1, 200, 255), 255, 130);
  	ofSetColor(c);
		ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		ofSetLineWidth(0.01);
  	ofDrawBezier(ofGetWidth() / 2, ofGetHeight() / 2, ofGetWidth() / 2 + (mouseX - ofGetWidth() / 2) / 3 + ofRandom(-volumeIn, volumeIn), ofGetHeight() / 2 + (mouseY - ofGetHeight() / 2) / 3 + ofRandom(-volumeIn, volumeIn), ofGetWidth() / 2 + (mouseX - ofGetWidth() / 2) * 2 / 3 + ofRandom(-volumeIn, volumeIn), ofGetHeight() / 2 + (mouseY - ofGetHeight() / 2) * 2 / 3 + ofRandom(-volumeIn, volumeIn), mouseX, mouseY);
		ofFill();
		c.setHsb(ofMap(hueValue_volumeIn, 50, 0, 0, 255), 255, 255, 80);
  	ofSetColor(c);
		ofEnableBlendMode(OF_BLENDMODE_ADD);
  	ofSetPolyMode(OF_POLY_WINDING_NONZERO);
  	ofBeginShape();
  	ofVertex(ofGetWidth() / 2 - CURSOR_QUAD_SIDE, ofGetHeight() / 2 + CURSOR_QUAD_SIDE);
  	ofVertex(ofGetWidth() / 2 + CURSOR_QUAD_SIDE, ofGetHeight() / 2 + CURSOR_QUAD_SIDE);
  	ofVertex(ofGetWidth() / 2 + CURSOR_QUAD_SIDE, ofGetHeight() / 2 - CURSOR_QUAD_SIDE);
  	ofVertex(ofGetWidth() / 2 - CURSOR_QUAD_SIDE, ofGetHeight() / 2 - CURSOR_QUAD_SIDE);
  	ofEndShape();
		c.setHsb(ofMap(hueValue_volumeIn, 0, 50, 0, 255), ofMap(volumeIn, -1, 1, 200, 255), 255, 130);
  	ofSetColor(c);
		ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		ofSetPolyMode(OF_POLY_WINDING_NONZERO);
  	ofBeginShape();
  	ofVertex(ofGetWidth() / 2 - (CURSOR_QUAD_SIDE - 1), ofGetHeight() / 2 + (CURSOR_QUAD_SIDE - 1));
  	ofVertex(ofGetWidth() / 2 + (CURSOR_QUAD_SIDE - 1), ofGetHeight() / 2 + (CURSOR_QUAD_SIDE - 1));
  	ofVertex(ofGetWidth() / 2 + (CURSOR_QUAD_SIDE - 1), ofGetHeight() / 2 - (CURSOR_QUAD_SIDE - 1));
  	ofVertex(ofGetWidth() / 2 - (CURSOR_QUAD_SIDE - 1), ofGetHeight() / 2 - (CURSOR_QUAD_SIDE - 1));
  	ofEndShape();
		ofEnableBlendMode(OF_BLENDMODE_ADD);
		ofSetLineWidth(1.5);
		ofDrawBezier(ofGetWidth() / 2, 0, ofGetWidth() / 2 + ofRandom(-volumeIn, volumeIn), ofGetHeight() / 3, ofGetWidth() / 2 + ofRandom(-volumeIn, volumeIn), ofGetHeight() * 2 / 3, ofGetWidth() / 2, ofGetHeight());
		ofDrawBezier(0, ofGetHeight() / 2, ofGetWidth() / 3, ofGetHeight() / 2 + ofRandom(-volumeIn, volumeIn), ofGetWidth() * 2 / 3, ofGetHeight() / 2 + ofRandom(-volumeIn, volumeIn), ofGetWidth(), ofGetHeight() / 2);
		ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		ofSetLineWidth(1.0);
		ofDrawBezier(ofGetWidth() / 2, 0, ofGetWidth() / 2 + ofRandom(-volumeIn, volumeIn), ofGetHeight() / 3, ofGetWidth() / 2 + ofRandom(-volumeIn, volumeIn), ofGetHeight() * 2 / 3, ofGetWidth() / 2, ofGetHeight());
		ofDrawBezier(0, ofGetHeight() / 2, ofGetWidth() / 3, ofGetHeight() / 2 + ofRandom(-volumeIn, volumeIn), ofGetWidth() * 2 / 3, ofGetHeight() / 2 + ofRandom(-volumeIn, volumeIn), ofGetWidth(), ofGetHeight() / 2);
		c.setHsb(ofMap(hueValue_volumeIn, 50, 0, 0, 255), 255, 255, 130);
  	ofSetColor(c);
		ofEnableBlendMode(OF_BLENDMODE_ADD);
  	ofSetPolyMode(OF_POLY_WINDING_NONZERO);
  	ofBeginShape();
  	ofVertex(mouseX - CURSOR_QUAD_SIDE + ofRandom(-volumeIn * spread_mul, volumeIn * spread_mul), mouseY + CURSOR_QUAD_SIDE + ofRandom(-volumeIn * spread_mul, volumeIn * spread_mul));
  	ofVertex(mouseX + CURSOR_QUAD_SIDE + ofRandom(-volumeIn * spread_mul, volumeIn * spread_mul), mouseY + CURSOR_QUAD_SIDE + ofRandom(-volumeIn * spread_mul, volumeIn * spread_mul));
  	ofVertex(mouseX + CURSOR_QUAD_SIDE + ofRandom(-volumeIn * spread_mul, volumeIn * spread_mul), mouseY - CURSOR_QUAD_SIDE + ofRandom(-volumeIn * spread_mul, volumeIn * spread_mul));
  	ofVertex(mouseX - CURSOR_QUAD_SIDE + ofRandom(-volumeIn * spread_mul, volumeIn * spread_mul), mouseY - CURSOR_QUAD_SIDE + ofRandom(-volumeIn * spread_mul, volumeIn * spread_mul));
  	ofEndShape();
		c.setHsb(ofMap(hueValue_volumeIn, 0, 50, 0, 255), ofMap(volumeIn, -1, 1, 200, 255), 255, 130);
  	ofSetColor(c);
		ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		ofSetPolyMode(OF_POLY_WINDING_NONZERO);
  	ofBeginShape();
  	ofVertex(mouseX - (CURSOR_QUAD_SIDE - 1) + ofRandom(-volumeIn * spread_mul, volumeIn * spread_mul), mouseY + (CURSOR_QUAD_SIDE - 1) + ofRandom(-volumeIn * spread_mul, volumeIn * spread_mul));
  	ofVertex(mouseX + (CURSOR_QUAD_SIDE - 1) + ofRandom(-volumeIn * spread_mul, volumeIn * spread_mul), mouseY + (CURSOR_QUAD_SIDE - 1) + ofRandom(-volumeIn * spread_mul, volumeIn * spread_mul));
  	ofVertex(mouseX + (CURSOR_QUAD_SIDE - 1) + ofRandom(-volumeIn * spread_mul, volumeIn * spread_mul), mouseY - (CURSOR_QUAD_SIDE - 1) + ofRandom(-volumeIn * spread_mul, volumeIn * spread_mul));
  	ofVertex(mouseX - (CURSOR_QUAD_SIDE - 1) + ofRandom(-volumeIn * spread_mul, volumeIn * spread_mul), mouseY - (CURSOR_QUAD_SIDE - 1) + ofRandom(-volumeIn * spread_mul, volumeIn * spread_mul));
  	ofEndShape();
	}
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  unsigned int i = 0;
  while(i<COLUMN){
    unsigned int j = 0;
    while(j<ROW){
      ofColor c;
      c.setHsb(ofMap(hueValue, 0, 50, 0, 255), ofMap(calc_mesh[1][i][j], -1, 1, 0, 255), ofMap(calc_mesh[1][i][j], -1, 1, 0, 255), 8);
      ofSetColor(c);
      ofDrawRectangle(i*ofGetWidth()/(double)ROW, j*ofGetHeight()/(double)COLUMN, ofGetWidth()/(double)ROW, ofGetHeight()/(double)COLUMN);
      ++j;
    }
    ++i;
  }
  i = 0;
  while(i<particles.size()){
    ofColor c;
    c.setHsb(ofMap(particles[i].erapsedTime, 0, 60, 0, 255), 255, 255, ofMap(particles[i].erapsedTime, 0, 60, 0, 255));
    ofSetColor(c);
		ofEnableBlendMode(OF_BLENDMODE_ADD);
		particles[i].addRadius(0.0);
    particles[i].draw();
		ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		particles[i].addRadius(-2.9);
    particles[i].draw();
		if(i<particles.size()-1){
			//Point to Point Lines
			ofDrawLine(particles[i].position.x, particles[i].position.y, particles[i+1].position.x, particles[i+1].position.y);
			if(i > 30){
				ofEnableBlendMode(OF_BLENDMODE_ADD);
				ofSetLineWidth(1.5);
				ofDrawLine(particles[i-29].position.x, particles[i-29].position.y, particles[i].position.x, particles[i].position.y);
				ofEnableBlendMode(OF_BLENDMODE_ALPHA);
				ofSetLineWidth(1.0);
				ofDrawLine(particles[i-29].position.x, particles[i-29].position.y, particles[i].position.x, particles[i].position.y);
			}
			//Quad Primitives
			if(i > 60){
				c.setHsb(ofMap(particles[i].erapsedTime, 0, 53, 0, 255), 255, 6, ofMap(particles[i].erapsedTime, 0, 70, 0, 255));
		    ofSetColor(c);
				ofEnableBlendMode(OF_BLENDMODE_ADD);
				ofSetPolyMode(OF_POLY_WINDING_NONZERO);
		  	ofBeginShape();
		  	ofVertex(particles[i].position.x, particles[i].position.y);
		  	ofVertex(particles[i-4-ofMap(volumeIn, -1, 1, 0, 7)].position.x, particles[i-4-ofMap(volumeIn, -1, 1, 0, 7)].position.y);
		  	ofVertex(particles[i-8-ofMap(volumeIn, -1, 1, 0, 7)].position.x, particles[i-8-ofMap(volumeIn, -1, 1, 0, 7)].position.y);
		  	ofVertex(particles[i-12-ofMap(volumeIn, -1, 1, 0, 7)].position.x, particles[i-12-ofMap(volumeIn, -1, 1, 0, 7)].position.y);
		  	ofEndShape();
			}
		}
    ++i;
  }
}

//--------------------------------------------------------------
void ofApp::exit(ofEventArgs &args){
  soundStream.close();
  particles.clear();
  //Memory Delete
  unsigned int i = 0;
  while(i<TIME){
    unsigned int j = 0;
    while(j<COLUMN){
      delete[] calc_mesh[i][j];
      ++j;
    }
    delete[] calc_mesh[i];
    ++i;
  }
  delete[] calc_mesh;
}

//--------------------------------------------------------------
void ofApp::audioIn(ofSoundBuffer &buffer){
		volumeIn = buffer.getSample(1, 1), volumeAve = buffer.getRMSAmplitude();
		if(senderSetup){
			ofxOscMessage m;
			m.setAddress("/mic/volume");
			m.addFloatArg(volumeIn);
			m.addFloatArg(volumeAve);
			oscSender.sendMessage(m, false);
		}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
	mouseVelocityX = x - lastX, mouseVelocityY = y - lastY;
	if(senderSetup){
	 ofxOscMessage m;
	 m.setAddress("/mouse/position");
	 m.addIntArg(x);
	 m.addIntArg(y);
	 m.addIntArg(mouseVelocityX);
	 m.addIntArg(mouseVelocityY);
	 oscSender.sendMessage(m, false);
  }
	cursor_draw = true;
  unsigned int i = x * ROW / (double)ofGetWidth() + 1, j = y * COLUMN / (double)ofGetHeight() + 1;
  if(i <= 1 || i >= ROW-1 || j <= 1 || j >= COLUMN-1){
    return;
  }
	double meshAddValue = 20.0 * volumeAve;
	if(senderSetup){
		ofxOscMessage m;
		m.setAddress("/calcmesh/currentvalue");
		m.addIntArg(i);
		m.addIntArg(j);
		m.addDoubleArg(meshAddValue);
		oscSender.sendMessage(m, false);
	}
	calc_mesh[1][i][j] += meshAddValue;
  particles.push_back(Particle(x, y, 0.2 * mouseVelocityX, 0.2 * mouseVelocityY));
	particles.push_back(Particle(x, y, ofRandom(-0.01 * mouseVelocityY, 0.01 * mouseVelocityY), ofRandom(-0.01 * mouseVelocityX, 0.01 * mouseVelocityX)));
  lastX = x, lastY = y;
}

//--------------------------------------------------------------
void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY){
    wheelValue += scrollY;
		if(wheelValue < 0.0){
      wheelValue = 0.0;
    }else if(wheelValue > 50.0){
      wheelValue = 50.0;
    }
		if(senderSetup){
		 ofxOscMessage m;
		 m.setAddress("/mouse/wheelvalue");
		 m.addIntArg(wheelValue);
		 oscSender.sendMessage(m, false);
	  }
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
  clickX = x, clickY = y, lastX = x, lastY = y;
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
	cursor_draw = false, mouseVelocityX = 0.0, mouseVelocityY = 0.0;
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}
