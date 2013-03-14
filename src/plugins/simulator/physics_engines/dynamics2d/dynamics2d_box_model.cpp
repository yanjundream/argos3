/**
 * @file <argos3/plugins/simulator/physics_engines/dynamics2d/dynamics2d_box_model.cpp>
 *
 * @author Carlo Pinciroli - <ilpincy@gmail.com>
 */

#include "dynamics2d_box_model.h"
#include "dynamics2d_gripping.h"
#include "dynamics2d_engine.h"

namespace argos {

   /****************************************/
   /****************************************/

   CDynamics2DBoxModel::CDynamics2DBoxModel(CDynamics2DEngine& c_engine,
                                            CBoxEntity& c_entity) :
      CDynamics2DModel(c_engine, c_entity.GetEmbodiedEntity()),
      m_cBoxEntity(c_entity),
      m_pcGrippable(NULL),
      m_fMass(c_entity.GetMass()),
      m_ptShape(NULL),
      m_ptBody(NULL) {
      /* Get the size of the entity */
      CVector3 cHalfSize = c_entity.GetSize() * 0.5f;
      /* Create a polygonal object in the physics space */
      /* Start defining the vertices
         NOTE: points must be defined in a clockwise winding
      */
      cpVect tVertices[] = {
         cpv(-cHalfSize.GetX(), -cHalfSize.GetY()),
         cpv(-cHalfSize.GetX(),  cHalfSize.GetY()),
         cpv( cHalfSize.GetX(),  cHalfSize.GetY()),
         cpv( cHalfSize.GetX(), -cHalfSize.GetY())
      };
      const CVector3& cPosition = GetEmbodiedEntity().GetPosition();
      CRadians cXAngle, cYAngle, cZAngle;
      GetEmbodiedEntity().GetOrientation().ToEulerAngles(cZAngle, cYAngle, cXAngle);
      if(c_entity.GetEmbodiedEntity().IsMovable()) {
         /* The box is movable */
         /* Create the body */
         m_ptBody =
            cpSpaceAddBody(m_cDyn2DEngine.GetPhysicsSpace(),
                           cpBodyNew(m_fMass,
                                     cpMomentForPoly(m_fMass,
                                                     4,
                                                     tVertices,
                                                     cpvzero)));
         m_ptBody->p = cpv(cPosition.GetX(), cPosition.GetY());
         cpBodySetAngle(m_ptBody, cZAngle.GetValue());
         /* Create the shape */
         m_ptShape =
            cpSpaceAddShape(m_cDyn2DEngine.GetPhysicsSpace(),
                            cpPolyShapeNew(m_ptBody,
                                           4,
                                           tVertices,
                                           cpvzero));
         m_ptShape->e = 0.0; // no elasticity
         m_ptShape->u = 0.7; // lots contact friction to help pushing
         /* The shape is grippable */
         m_pcGrippable = new CDynamics2DGrippable(GetEmbodiedEntity(),
                                                  m_ptShape);
         /* Friction with ground */
         m_ptLinearFriction =
            cpSpaceAddConstraint(m_cDyn2DEngine.GetPhysicsSpace(),
                                 cpPivotJointNew2(m_cDyn2DEngine.GetGroundBody(),
                                                  m_ptBody,
                                                  cpvzero,
                                                  cpvzero));
         m_ptLinearFriction->maxBias = 0.0f; // disable joint correction
         m_ptLinearFriction->maxForce = 1.49f; // emulate linear friction (this is just slightly smaller than FOOTBOT_MAX_FORCE)
         m_ptAngularFriction =
            cpSpaceAddConstraint(m_cDyn2DEngine.GetPhysicsSpace(),
                                 cpGearJointNew(m_cDyn2DEngine.GetGroundBody(),
                                                m_ptBody,
                                                0.0f,
                                                1.0f));
         m_ptAngularFriction->maxBias = 0.0f; // disable joint correction
         m_ptAngularFriction->maxForce = 1.49f; // emulate angular friction (this is just slightly smaller than FOOTBOT_MAX_TORQUE)
      }
      else {
         /* The box is not movable */
         /* Manually rotate the vertices */
         cpVect tRot = cpvforangle(cZAngle.GetValue());
         tVertices[0] = cpvrotate(tVertices[0], tRot);
         tVertices[1] = cpvrotate(tVertices[1], tRot);
         tVertices[2] = cpvrotate(tVertices[2], tRot);
         tVertices[3] = cpvrotate(tVertices[3], tRot);
         /* Create the shape */
         m_ptShape =
            cpSpaceAddStaticShape(m_cDyn2DEngine.GetPhysicsSpace(),
                                  cpPolyShapeNew(m_cDyn2DEngine.GetGroundBody(),
                                                 4,
                                                 tVertices,
                                                 cpv(cPosition.GetX(), cPosition.GetY())));
         
         m_ptShape->e = 0.0; // No elasticity
         m_ptShape->u = 0.1; // Little contact friction to help sliding away
         /* This shape is normal (not grippable, not gripper) */
         m_ptShape->collision_type = CDynamics2DEngine::SHAPE_NORMAL;
      }
      /* Calculate bounding box */
      GetBoundingBox().MinCorner.SetZ(GetEmbodiedEntity().GetPosition().GetZ());
      GetBoundingBox().MaxCorner.SetZ(GetEmbodiedEntity().GetPosition().GetZ() + m_cBoxEntity.GetSize().GetZ());
      CalculateBoundingBox();
   }
   
   /****************************************/
   /****************************************/

   CDynamics2DBoxModel::~CDynamics2DBoxModel() {
      if(m_ptBody != NULL) {
         delete m_pcGrippable;
         cpSpaceRemoveConstraint(m_cDyn2DEngine.GetPhysicsSpace(), m_ptLinearFriction);
         cpSpaceRemoveConstraint(m_cDyn2DEngine.GetPhysicsSpace(), m_ptAngularFriction);
         cpConstraintFree(m_ptLinearFriction);
         cpConstraintFree(m_ptAngularFriction);
         cpSpaceRemoveShape(m_cDyn2DEngine.GetPhysicsSpace(), m_ptShape);
         cpSpaceRemoveBody(m_cDyn2DEngine.GetPhysicsSpace(), m_ptBody);
         cpShapeFree(m_ptShape);
         cpBodyFree(m_ptBody);
      }
      else {
         cpSpaceRemoveStaticShape(m_cDyn2DEngine.GetPhysicsSpace(), m_ptShape);
         cpShapeFree(m_ptShape);
         cpSpaceReindexStatic(m_cDyn2DEngine.GetPhysicsSpace());
      }
   }

   /****************************************/
   /****************************************/

   bool CDynamics2DBoxModel::CheckIntersectionWithRay(Real& f_t_on_ray,
                                                       const CRay3& c_ray) const {
      cpSegmentQueryInfo tInfo;
      if(cpShapeSegmentQuery(m_ptShape,
                             cpv(c_ray.GetStart().GetX(), c_ray.GetStart().GetY()),
                             cpv(c_ray.GetEnd().GetX()  , c_ray.GetEnd().GetY()  ),
                             &tInfo)) {
      	 CVector3 cIntersectionPoint;
      	 c_ray.GetPoint(cIntersectionPoint, tInfo.t);
      	 if((cIntersectionPoint.GetZ() >= GetEmbodiedEntity().GetPosition().GetZ()) &&
      			(cIntersectionPoint.GetZ() <= GetEmbodiedEntity().GetPosition().GetZ() + m_cBoxEntity.GetSize().GetZ()) ) {
            f_t_on_ray = tInfo.t;
            return true;
      	 }
      	 else {
            return false;
      	 }
      }
      else {
         return false;
      }
   }

   /****************************************/
   /****************************************/

   bool CDynamics2DBoxModel::MoveTo(const CVector3& c_position,
                                     const CQuaternion& c_orientation,
                                     bool b_check_only) {
      SInt32 nCollision;
      /* Check whether the box is movable or not */
      if(m_cBoxEntity.GetEmbodiedEntity().IsMovable()) {
         /* The box is movable */
         /* Save body position and orientation */
         cpVect tOldPos = m_ptBody->p;
         cpFloat fOldA = m_ptBody->a;
         /* Move the body to the desired position */
         m_ptBody->p = cpv(c_position.GetX(), c_position.GetY());
         CRadians cXAngle, cYAngle, cZAngle;
         c_orientation.ToEulerAngles(cZAngle, cYAngle, cXAngle);
         cpBodySetAngle(m_ptBody, cZAngle.GetValue());
         /* Create a shape sensor to test the movement */
         /* First construct the vertices */
         CVector3 cHalfSize = m_cBoxEntity.GetSize() * 0.5f;
         cpVect tVertices[] = {
            cpv(-cHalfSize.GetX(), -cHalfSize.GetY()),
            cpv(-cHalfSize.GetX(),  cHalfSize.GetY()),
            cpv( cHalfSize.GetX(),  cHalfSize.GetY()),
            cpv( cHalfSize.GetX(), -cHalfSize.GetY())
         };
         /* Then create the shape itself */
         cpShape* ptTestShape = cpPolyShapeNew(m_ptBody,
                                               4,
                                               tVertices,
                                               cpvzero);
         /* Check if there is a collision */
         nCollision = cpSpaceShapeQuery(m_cDyn2DEngine.GetPhysicsSpace(), ptTestShape, NULL, NULL);
         /* Dispose of the sensor shape */
         cpShapeFree(ptTestShape);
         if(b_check_only || nCollision) {
            /* Restore old body state if there was a collision or
               it was only a check for movement */
            m_ptBody->p = tOldPos;
            cpBodySetAngle(m_ptBody, fOldA);
         }
         else {
            /* Object moved, release all gripped entities */
            m_pcGrippable->ReleaseAll();
            /* Update the active space hash if the movement is actual */
            cpSpaceReindexShape(m_cDyn2DEngine.GetPhysicsSpace(), m_ptShape);
            /* Update bounding box */
            CalculateBoundingBox();
         }
      }
      else {
         /* The box is not movable, so you can't move it :-) */
         nCollision = 1;
      }
      /* The movement is allowed if there is no collision */
      return !nCollision;
   }

   /****************************************/
   /****************************************/

   void CDynamics2DBoxModel::Reset() {
      if(m_ptBody != NULL) {
         /* Reset body position */
         const CVector3& cPosition = GetEmbodiedEntity().GetInitPosition();
         m_ptBody->p = cpv(cPosition.GetX(), cPosition.GetY());
         /* Reset body orientation */
         CRadians cXAngle, cYAngle, cZAngle;
         GetEmbodiedEntity().GetInitOrientation().ToEulerAngles(cZAngle, cYAngle, cXAngle);
         cpBodySetAngle(m_ptBody, cZAngle.GetValue());
         /* Zero speed and applied forces */
         m_ptBody->v = cpvzero;
         m_ptBody->w = 0.0f;
         cpBodyResetForces(m_ptBody);
         /* Update bounding box */
         cpShapeCacheBB(m_ptShape);
         CalculateBoundingBox();
         /* Release all attached entities */
         m_pcGrippable->ReleaseAll();
      }
   }

   /****************************************/
   /****************************************/

   void CDynamics2DBoxModel::CalculateBoundingBox() {
      GetBoundingBox().MinCorner.SetX(m_ptShape->bb.l);
      GetBoundingBox().MinCorner.SetY(m_ptShape->bb.b);
      GetBoundingBox().MaxCorner.SetX(m_ptShape->bb.r);
      GetBoundingBox().MaxCorner.SetY(m_ptShape->bb.t);
   }

   /****************************************/
   /****************************************/

   void CDynamics2DBoxModel::UpdateEntityStatus() {
      if(m_ptBody != NULL) {
         /* Update bounding box */
         CalculateBoundingBox();
         /* Update entity position and orientation */
         m_cDyn2DEngine.PositionPhysicsToSpace(m_cSpacePosition, GetEmbodiedEntity().GetPosition(), m_ptBody);
         GetEmbodiedEntity().SetPosition(m_cSpacePosition);
         m_cDyn2DEngine.OrientationPhysicsToSpace(m_cSpaceOrientation, m_ptBody);
         GetEmbodiedEntity().SetOrientation(m_cSpaceOrientation);
      }
      /* Update components */
      m_cBoxEntity.UpdateComponents();
   }

   /****************************************/
   /****************************************/

   bool CDynamics2DBoxModel::IsCollidingWithSomething() const {
      return cpSpaceShapeQuery(m_cDyn2DEngine.GetPhysicsSpace(), m_ptShape, NULL, NULL) > 0;
   }

   /****************************************/
   /****************************************/

   REGISTER_STANDARD_DYNAMICS2D_OPERATIONS_ON_ENTITY(CBoxEntity, CDynamics2DBoxModel);

   /****************************************/
   /****************************************/

}
