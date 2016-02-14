#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <Core/Updatable.h>

using namespace std;

/** @brief a first person Camera */
class Camera : public Updatable{
protected:
	glm::vec3 m_position;			/**< current world position */
	glm::vec3 m_direction;			/**< current world normalized view direction */
	glm::mat4 m_viewMatrix;   		/**< current view matrix */
	glm::mat4 m_projectionMatrix; 	/**< current projection matrix */

	float m_phi;			/**< rotation, horizontal */
	float m_theta;			/**< inclination, vertical */
	float m_speedRight;		/**< horizontal velocity */
	float m_speedForward;	/**< forward velocity */

	bool m_topDown;			/**< bool to decide whether up vector should be inverted */

	/** \brief returns an Up vector
	 * @return Up vector
	 */
	glm::vec3 getUp();

	/** \brief clamps Phi and Thetha 
	 *
	 * clip Phi, Theta into [-PI; PI] and [0, 2*PI]
	 */
	inline void clampPhiTheta();

public:
	/** \brief construct a camera in at default position
	 *	default position is ( 0.0, 0.0, 5.0 ) 
	 *	default view direction is negatie z axis
	 *  default velocity is 0.0
	 */
	Camera();

	/** \brief destructor
	 *
	 */
	~Camera();

	/* GETTER AND SETTER BEGIN */

	/** \brief getter
	 *
	 * get rotational angle (yaw), always within [0,2*PI]
	 * @return float value
	 */
	float getPhi();

	/** \brief setter
	 *
	 * set phi to provided float, will be clamped to [0,2*PI]
	 * @param updatePhi
	 */
	void setPhi(float updatePhi);

	/** \brief getter
	 *
	 * get inclinational angle (pitch), always within ]-PI , PI[
	 * @return float value
	 */
	float getTheta();

	/** \brief setter
	 *
	 * set theta to provided float, will be clamped to ] -PI, PI []
	 * @param updateTheta
	 */
	void setTheta(float updateTheta);

	/** \brief setter
	 *
	 * sets/changes speedRight
	 * @param speed
	 */
	void setSpeedRight(float speed);

	/** \brief setter
	 *
	 * sets/changes speedForward
	 * @param speed
	 */
	void setSpeedForward(float speed);

	/** \brief update position by evaluating current velocity over provided time step
	 * @param deltaTime : time step in seconds
	 */
	virtual void updatePosition(float deltaTime);

	/** \brief update position by evaluating current velocity over provided time step
	* @param d_t : time step in seconds
	*/
	virtual void update(float d_t = 0.1f);

	/** \brief getter
	 *
	 * @return speedRight
	 */
	float getSpeedRight();

	/** \brief getter
	 *
	 * @return speedForward
	 */
	float getSpeedForward();

	/** \brief getter
	 *
	 * @return right
	 */
	glm::vec3 getRight();

	/** \brief getter
	 *
	 * @return the viewDirection (direction)
	 */
	glm::vec3 getViewDirection();

	/** \brief getter
	 *
	 * @return position
	 */
	glm::vec3 getPosition();

	/** \brief getter
	 *
	 * @return position pointer
	 */
	glm::vec3* getPositionPointer();

	/** \brief setter
	 *
	 * sets/changes the x,y,and z value of position
	 * @param x,y,z position as float values
	 */
	virtual void setPosition(float x, float y, float z);

	/** \brief setter
	 *
	 * sets/changes the x,y,and z value of position
	 * @param newPos position as glm vec3
	 */
	virtual void setPosition(glm::vec3 newPos);

	/** \brief setter
	 *
	 * sets/changes direction
	 * @param dir
	 */
	virtual void setDirection(glm::vec3 dir);

	/** \brief setter
	 *
	 * sets/changes center
	 * @param center
	 */
	void setCenter(glm::vec3 center);
	/* GETTER AND SETTER END */

	/** \brief compute normalized view direction from current inclination and rotation
	 *
	 * compute ViewDirection from Phi and Theta
	 */
	inline void updateViewDirection();

	/** \brief compute inclination and rotation angles from current view direction
	 *
	 * compute Phi and Theta from View Direction
	 */
	inline void updatePhiTheta();

	/** \brief getter
	 *
	 * compute ViewMatrix
	 * @return viewMatrix
	 */
	virtual glm::mat4 getViewMatrix();


	/** \brief getter
	 *
	 * @return viewMatrixPointer
	 */
	virtual glm::mat4* getViewMatrixPointer();

	/** \brief getter
	 *
	 * @return projectionMatrixPointer
	 */
	glm::mat4 getProjectionMatrix();

	/** \brief getter
	 *
	 * @return projectionMatrixPointer
	 */
	glm::mat4* getProjectionMatrixPointer();

	/** \brief setter
	 *
	 * set projection matrix
	 */
	void setProjectionMatrix( glm::mat4 projectionMatrix );

	/** \brief setter
	 * 
	 * set top-down boolean
	 */
	void setTopDown(bool to);

	/** \brief getter
	 * 
	 * get top-down boolean
	 */
	bool getTopDown();

	//static int s_key_forward, s_key_back, s_key_left, s_key_right;
	void keyboardControlCallback(int k, int s, int a, int m);
	std::function<void(int,int,int,int)> getKeyboardControlCallbackFunction();
	void mouseControlCallback(double d_theta, double d_phi);
	std::function<void(double,double)> getMouseControlCallbackFunction();
};

#endif
