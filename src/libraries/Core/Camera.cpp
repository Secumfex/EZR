#include "Camera.h"

#define PI 3.14159265f

Camera::Camera(){
	m_position = glm::vec3(0.0f, 0.0f, 0.0f);

	// Initial horizontal angle : toward -Z
	m_phi = PI;
	// Initial vertical angle : none
	m_theta = 0.0f;

	m_speedRight = 0.0f;
	m_speedForward = 0.0f;

	m_topDown = false;	// 

	//compute initial View Direction vector
	updateViewDirection();
	m_viewMatrix = getViewMatrix();
	m_projectionMatrix = glm::ortho(-0.5f,0.5f,-0.5f,0.5f);

	m_mouseSensitivity = 1.0f / 300.0f;
}

Camera::~Camera(){}


/* GETTER AND SETTER BEGIN */
float Camera::getPhi(){
	return m_phi;
}

void Camera::setPhi(float updatePhi){
	m_phi = updatePhi;
	updateViewDirection();
}

float Camera::getTheta(){
	return m_theta;
}

void Camera::setTheta(float updateTheta){
	m_theta = updateTheta;
	updateViewDirection();
}

void Camera::setSpeedRight(float speed){
	m_speedRight = speed;
}

float Camera::getSpeedRight(){
	return m_speedRight;
}

float Camera::getSpeedForward(){
	return m_speedForward;
}

void Camera::setSpeedForward(float speed){
	m_speedForward = speed;
}

void Camera::updatePosition(float deltaTime){
	glm::vec3 gotPosition = getPosition();
	gotPosition += getViewDirection() * deltaTime * m_speedForward;
	gotPosition += getRight() * deltaTime * m_speedRight;
	setPosition(gotPosition);
}

void Camera::update(float d_t)
{
	updatePosition(d_t);
}

glm::vec3 Camera::getViewDirection(){
	return m_direction;
}

glm::vec3 Camera::getPosition(){
	return m_position;
}

glm::vec3* Camera::getPositionPointer(){
	return &m_position;
}

void Camera::setPosition(float x, float y, float z){
	m_position = glm::vec3(x,y,z);
}

void Camera::setPosition(glm::vec3 newPos){
	m_position = newPos;
}

/* GETTER AND SETTER END */

// Direction : Spherical coordinates to Cartesian coordinates conversion
void Camera::updateViewDirection(){
	clampPhiTheta();
	m_direction = glm::vec3(	cos(m_theta) * sin(m_phi),
							sin(m_theta),
							cos(m_phi) * cos(m_theta)	);
	m_direction = glm::normalize(m_direction);
}

// assumption: direction is normalized
void Camera::updatePhiTheta(){
	m_theta 	= std::atan2(m_direction.y,std::sqrt(m_direction.x *m_direction.x + m_direction.z * m_direction.z)); // inclination 
	m_phi 	= std::atan2(m_direction.x, m_direction.z); // rotation
}

	// Right vector
glm::vec3 Camera::getRight(){
	glm::vec3 right = glm::vec3(	sin(m_phi - PI / 2.0f),
									0,
									cos(m_phi - PI / 2.0f));
	right = glm::normalize(right);
	return right;
}
	// Up vector
glm::vec3 Camera::getUp(){
	glm::vec3 up = glm::cross(getRight() , m_direction);
	up = glm::normalize(up);
	if (m_topDown){
		up *= -1.0f;
	}
	return up;
}

glm::mat4 Camera::getViewMatrix(){
	// Camera matrix
	m_viewMatrix =glm::lookAt(
		m_position,           // Camera is here
		m_position + m_direction, // and looks here : at the same position, plus "direction"
		getUp()                  // Head is up (set to 0,-1,0 to look upside-down)
		); 
	return m_viewMatrix;
	}

glm::mat4* Camera::getViewMatrixPointer(){
	return &m_viewMatrix;
}

void Camera::setProjectionMatrix( glm::mat4 projectionMatrix )
{
	m_projectionMatrix = projectionMatrix;
}

glm::mat4 Camera::getProjectionMatrix()
{
	return m_projectionMatrix;
}

glm::mat4* Camera::getProjectionMatrixPointer() {
	return &m_projectionMatrix;
}

void Camera::setDirection(glm::vec3 dir){
	m_direction = glm::normalize(dir);
	updatePhiTheta();	// update phi & theta by evaluating the new direction
}

void Camera::setCenter(glm::vec3 center){
	m_direction = center - m_position;
	m_direction = glm::normalize(m_direction);
	updatePhiTheta();	// update phi & theta by evaluating the new direction
}

void Camera::clampPhiTheta(){
	m_phi = std::fmod (m_phi , 2.0f * PI);

	if (m_theta >= PI / 2.0f){
		m_theta =  PI / 2.0f - 0.001f;
	}
	if (m_theta <= -PI / 2.0f ){
		m_theta = -PI / 2.0f + 0.001f;
	}
}

void Camera::setTopDown(bool to){
	m_topDown = to;
}

bool Camera::getTopDown(){
	return m_topDown;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//#include <GLFW/glfw3.h>
// identical defines cuz header include causes errors
#define _GLFW_KEY_W 87
#define _GLFW_KEY_S 83
#define _GLFW_KEY_A 65
#define _GLFW_KEY_D 68
#define _GLFW_REPEAT 2
#define _GLFW_RELEASE 0

void Camera::keyboardControlCallback(int k, int s, int a, int m)
{
	if (a == _GLFW_REPEAT) {return;} 
	switch(k){
	case _GLFW_KEY_W:
		setSpeedForward( ( a == _GLFW_RELEASE) ? getSpeedForward() - 3.0f : getSpeedForward() + 3.0f);
	 	break;
	case _GLFW_KEY_A:
		setSpeedRight( ( a == _GLFW_RELEASE) ? getSpeedRight() + 3.0f : getSpeedRight() - 3.0f);
	 	break;
	case _GLFW_KEY_S:
		setSpeedForward( ( a == _GLFW_RELEASE) ? getSpeedForward() + 3.0f : getSpeedForward() - 3.0f);
		break;
	case _GLFW_KEY_D:
		setSpeedRight( ( a == _GLFW_RELEASE) ? getSpeedRight() - 3.0f : getSpeedRight() + 3.0f);
	 	break;
	default:
	 	break;
	}
}

std::function<void(int,int,int,int)> Camera::getKeyboardControlCallbackFunction()
{
		return std::bind(
		&Camera::keyboardControlCallback,
		this, 
		std::placeholders::_1,
		std::placeholders::_2,
		std::placeholders::_3,
		std::placeholders::_4
		);
}

void Camera::mouseControlCallback(double d_theta, double d_phi)
{
	setTheta(getTheta() - d_theta * m_mouseSensitivity);
	setPhi( getPhi() - d_phi * m_mouseSensitivity);
}

std::function<void(double, double)> Camera::getMouseControlCallbackFunction()
{
		return std::bind(
		&Camera::mouseControlCallback,
		this, 
		std::placeholders::_1,
		std::placeholders::_2
		);
}
////////////////////////////////////////////////////////////////////////////////////////////////