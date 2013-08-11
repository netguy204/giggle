/*
 *  This file is part of GambitGameLib.
 *
 *  GambitGameLib is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GambitGameLib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GambitGameLib.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "platform.h"
#include "config.h"

#include <math.h>
#include <stdarg.h>

OBJECT_IMPL(CPlatformer, Component);
OBJECT_PROPERTY(CPlatformer, w);
OBJECT_PROPERTY(CPlatformer, h);
OBJECT_PROPERTY(CPlatformer, density);
OBJECT_PROPERTY(CPlatformer, friction);
OBJECT_PROPERTY(CPlatformer, parent);
OBJECT_PROPERTY(CPlatformer, offset);

CPlatformer::CPlatformer(void* go)
  : Component((GO*)go, PRIORITY_THINK),
    fixture(NULL), parent(NULL), w(10), h(10),
    density(1), friction(0.2) {
  this->go->body->SetType(b2_dynamicBody);
  vector_zero(&offset);
}

CPlatformer::~CPlatformer() {
  go->body->DestroyFixture(fixture);
}

void CPlatformer::init() {
  // build a pill shaped collider so that we don't get stuck on seams
  float hw = (w/2) / BSCALE;
  float hh = (h/2) / BSCALE;
  float tweak = (w * 0.1) / BSCALE;
  float ox = offset.x / BSCALE;
  float oy = offset.y / BSCALE;

  b2PolygonShape cornerBox;
  b2Vec2 corners[] = {
    b2Vec2(-hw + tweak + ox, -hh + oy),
    b2Vec2(hw - tweak + ox, -hh + oy),
    b2Vec2(hw + ox, -hh + tweak + oy),
    b2Vec2(hw + ox, hh + oy),
    b2Vec2(-hw + ox, hh + oy),
    b2Vec2(-hw + ox, -hh + tweak + oy)};
  cornerBox.Set(corners, 6);

  b2FixtureDef fixtureDef;
  fixtureDef.friction = friction;
  fixtureDef.density = density;
  fixtureDef.shape = &cornerBox;
  fixture = go->body->CreateFixture(&fixtureDef);
  fixture->SetUserData(this);
}

void CPlatformer::update(float dt) {
  // try to find a contact with our fixture
  b2ContactEdge* node = go->body->GetContactList();
  GO* old_parent = parent;
  Vector_ ourpos;
  go->pos(&ourpos);

  parent = NULL;

  while(node) {
    b2Fixture* fa = node->contact->GetFixtureA();
    b2Fixture* fb = node->contact->GetFixtureB();
    if(node->contact->IsTouching() &&
       (!fa->IsSensor() && !fb->IsSensor()) &&
       (fa == fixture ||
        fb == fixture)) {

      // need to ensure normal is up
      GO* other = (GO*)node->other->GetUserData();
      b2WorldManifold manifold;
      node->contact->GetWorldManifold(&manifold);

      // normal goes from A to B so we need to be careful
      if((node->contact->GetFixtureA() == fixture && manifold.normal.y < 0)
         || manifold.normal.y > 0) {

        // normal is pointing the right way but the contact must be
        // below us
        if((manifold.points[0].y*BSCALE) < ourpos.y) {
          parent = other;
          break;
        }
      }
    }
    node = node->next;
  }

  if(parent != old_parent) {
    go->send_message(go->create_message(MESSAGE_PARENT_CHANGE, NULL, 0));
  }
}
