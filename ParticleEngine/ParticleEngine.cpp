﻿#include "ParticleEngine.h"
#include "ForceGenerators.hpp"
#include "Config.hpp"

ParticleEngine::ParticleEngine()
{
	//Setting up Solid geometry
	Solid centerPlatform;
	centerPlatform.SetSize(sf::Vector2f((float)Config::width * 0.3f, (float)Config::height * 0.05f));
	centerPlatform.SetRotation(15.0f);
	centerPlatform.SetPosition(sf::Vector2f((float)Config::width * 0.5f, (float)Config::height * 0.5f));
	m_solids.push_back(centerPlatform);

	//BottemLeft Platform
	Solid leftBottomPlatform;
	leftBottomPlatform.SetSize(sf::Vector2f((float)Config::width * 0.3f, (float)Config::height * 0.4f));
	leftBottomPlatform.SetRotation(45.0f);
	leftBottomPlatform.SetPosition(sf::Vector2f((float)Config::width * 0.0f, (float)Config::height));
	m_solids.push_back(leftBottomPlatform);

	//Left Wall
	Solid wall; 
	wall.SetSize(sf::Vector2f((float)Config::width* 0.45f, (float)Config::height));
	wall.SetPosition(sf::Vector2f((float)Config::width * -0.2f, (float)Config::height * 0.50f));
	m_solids.push_back(wall);

	//Right Wall
	wall.SetPosition(sf::Vector2f((float)Config::width * 1.2f, (float)Config::height * 0.50f));
	m_solids.push_back(wall);

	//floor
	Solid floor;
	floor.SetSize(sf::Vector2f((float)Config::width, (float)Config::height * 0.45f));
	floor.SetPosition(sf::Vector2f((float)Config::width * 0.5f, (float)Config::height * 1.2f));
	m_solids.push_back(floor);

	//ceiling
	floor.SetPosition(sf::Vector2f((float)Config::width * 0.5f, (float)Config::height * -0.2f));
	m_solids.push_back(floor);

	//Setting up blizzards
	Blizzard blizzard1(glm::vec2((float)Config::width * 0.75f, (float)Config::height * 0.25f), 25);
	m_blizzards.push_back(blizzard1);

	Blizzard blizzard2(glm::vec2((float)Config::width * 0.25f, (float)Config::height * 0.25f), 25);
	m_blizzards.push_back(blizzard2);

	//Setting Up BallGenerator
	BallGenerator ballGen1(glm::vec2((float)Config::width * 0.25f, (float)Config::height * 0.15f), glm::vec2((float)Config::width * 0.75f, (float)Config::height * 0.15f), 10.0f);
	m_ballGenerators.push_back(ballGen1);

	//Setting up Fans
	Fan fan1(glm::vec2((float)Config::width * 0.95f, (float)Config::height * 0.15f), glm::vec2((float)Config::width * 0.95f, (float)Config::height * 0.35f), 10.0f);
	m_fans.push_back(fan1);

	Fan fan2(glm::vec2((float)Config::width * 0.95f, (float)Config::height * 0.99f), glm::vec2((float)Config::width * 0.75f, (float)Config::height * 0.99f), 20.0f);
	m_fans.push_back(fan2);

	m_particleVertices.reserve(Config::maxParticleCount);
	m_particles.reserve(Config::maxParticleCount);
	m_particleReflexions.reserve(Config::maxParticleCount);
	m_ballReflexions.reserve(Config::maxBallCount * m_solids.size());
	m_particleCollisions.reserve(Config::maxBallCount * Config::maxBallCount + Config::clothColumns * Config::clothRows);
	m_clothReflexions.reserve(Config::clothColumns * Config::clothRows);

	//Setting Up Rendering Stuff
	renderCircle.setPointCount(15);
	renderCircle.setRadius(1.0f);
	renderCircle.setOrigin(renderCircle.getRadius(), renderCircle.getRadius());
	renderCircle.setOutlineThickness(1.0f / Config::ballSize);
	renderCircle.setOutlineColor(sf::Color::Yellow);
	renderCircle.setFillColor(sf::Color::Transparent);

	GenerateCloth(10.0f, glm::vec2((float)Config::width * 0.15f, (float)Config::height * 0.45f), 5.0f);
}

ParticleEngine::~ParticleEngine()
{
}

void ParticleEngine::Update(float deltaTime)
{
	for (size_t i = 0u; i < m_blizzards.size(); ++i)
	{
		m_blizzards[i].Update(deltaTime, *this);
	}

	for (size_t i = 0u; i < m_ballGenerators.size(); ++i)
	{
		m_ballGenerators[i].Update(deltaTime, *this);
	}

	ApplyForces();
	Integrate(deltaTime);
	CheckCollisions();
	ResolveCollisions();
	DeleteParticles();
}

void ParticleEngine::Render(sf::RenderWindow& window)
{
	for (size_t i = 0u; i < m_solids.size(); ++i)
	{
		m_solids[i].Render(window);
	}

	for (size_t i = 0u; i < m_blizzards.size(); ++i)
	{
		m_blizzards[i].DebugRender(window);
	}

	for (size_t i = 0u; i < m_fans.size(); ++i)
	{
		m_fans[i].Render(window);
	}

	for (size_t i = 0u; i < m_ballGenerators.size(); ++i)
	{
		m_ballGenerators[i].Render(window);
	}

	//Cloth
	for (size_t i = Config::clothColumns; i < m_cloth.size(); ++i)
	{
		renderCircle.setPosition(m_cloth[i].position.x, m_cloth[i].position.y);
		renderCircle.setScale(m_cloth[i].radius, m_cloth[i].radius);
		window.draw(renderCircle);
	}
	
	//Springs
	for(size_t i = 0u; i < m_springs.size(); ++i)
	{
		m_springVertices[i*2].position.x = m_springs[i].p1->position.x;
		m_springVertices[i*2].position.y = m_springs[i].p1->position.y;
		m_springVertices[i*2+1].position.x = m_springs[i].p2->position.x;
		m_springVertices[i*2+1].position.y = m_springs[i].p2->position.y;
	}

	window.draw(&m_springVertices[0], m_springVertices.size(), sf::PrimitiveType::Lines);

	//Balls
	for (size_t i = 0u; i < m_balls.size(); ++i)
	{
		renderCircle.setPosition(m_balls[i].position.x, m_balls[i].position.y);
		renderCircle.setScale(m_balls[i].radius, m_balls[i].radius);
		window.draw(renderCircle);
	}

	//Particles
	for (size_t i = 0u; i < m_particles.size(); ++i)
	{
		m_particleVertices[i].position.x = m_particles[i].position.x;
		m_particleVertices[i].position.y = m_particles[i].position.y;
	}

	if(!m_particleVertices.empty())
	{
		window.draw(&m_particleVertices[0], m_particleVertices.size(), sf::PrimitiveType::Points);
	}
}

void ParticleEngine::AddParticle(const Particle& particle)
{
	if(m_particles.size() + 1 > Config::maxParticleCount)
	{
		m_particles.erase(m_particles.begin());
	}

	if (m_particleVertices.size() + 1 > Config::maxParticleCount)
	{
		m_particleVertices.erase(m_particleVertices.begin());
	}

	m_particles.push_back(particle);

	sf::Vertex vertex;
	vertex.position.x = particle.position.x;
	vertex.position.y = particle.position.y;
	vertex.color = sf::Color::White;

	m_particleVertices.push_back(vertex);
}

void ParticleEngine::AddBall(const Ball& ball)
{
	if (m_balls.size() + 1 > Config::maxBallCount)
	{
		m_balls.erase(m_balls.begin());
	}

	m_balls.push_back(ball);
}

void ParticleEngine::GetInput(const sf::Event::MouseButtonEvent& e)
{
	if(e.button == sf::Mouse::Button::Left)
	{
		Ball b1;
		b1.position = glm::vec2((float)e.x, (float)e.y);
		AddBall(b1);
	}
}

void ParticleEngine::AddSpringContraint(size_t p1Index, size_t p2Index)
{
	if(p2Index < m_cloth.size())
	{
		ForceGenerators::SpringContraint s;
		s.p1 = &m_cloth[p1Index];
		s.p2 = &m_cloth[p2Index];
		s.restLength = Collisions::saveDistance(m_cloth[p1Index].position, m_cloth[p2Index].position);
		s.stiffness = 2.0f;
		m_springs.push_back(s);
	}
}


void ParticleEngine::GenerateCloth(const float ballRadius, const glm::vec2& startPosition, float spacing)
{
	m_cloth.reserve(Config::clothColumns * Config::clothRows);
	glm::vec2 currentPosition = startPosition;

	spacing += ballRadius * 2.0f;

	for(size_t i = 0u; i < Config::clothRows; ++i)
	{
		for (size_t j = 0u; j < Config::clothColumns; ++j)
		{
			Ball b;
			b.radius = ballRadius;
			b.position = currentPosition;
			m_cloth.push_back(b);

			currentPosition.x += spacing;
		}

		currentPosition.x = startPosition.x;
		currentPosition.y += spacing;
	}

	for(size_t i = 0u; i < m_cloth.size(); ++i)
	{

		if(((int)i + 1) % (int)Config::clothColumns != 0)
		{
			//Right Neighbor
			AddSpringContraint(i, i + 1);
		}
		
		//Bottom Neighbor
		AddSpringContraint(i, i + Config::clothColumns);
	}

	m_springVertices.resize(m_springs.size() * 2);

	for(size_t i = 0u; i < m_springs.size(); ++i)
	{
		m_springVertices[i * 2].position.x = m_springs[i].p1->position.x;
		m_springVertices[i * 2].position.y = m_springs[i].p1->position.y;
		m_springVertices[i*2].color = sf::Color::Blue;
		m_springVertices[i*2+1].position.x = m_springs[i].p2->position.x;
		m_springVertices[i*2+1].position.y = m_springs[i].p2->position.y;
		m_springVertices[i * 2 + 1].color = sf::Color::Blue;
	}
}


void ParticleEngine::CheckCollisions()
{
	ForceGenerators::ParticleCollision collision;
	Collisions::Contact contact;

	for (size_t i = 0u; i < m_particles.size(); ++i)
	{
		for (size_t j = 0u; j < m_solids.size(); ++j)
		{
			if (Collisions::PointBoxCollision(m_particles[i].position, m_solids[j].aabb))
			{
				//Collision with AABB!
				if(Collisions::PointBoxCollision(m_solids[j].oobb, m_particles[i].position, contact))
				{
					//OOBB Collision!
					contact.index = i;
					m_particleReflexions.push_back(contact);
				}
			}
		}

		//Check Balls
		for (size_t j = 0u; j < m_balls.size(); ++j)
		{
			if(Collisions::PointSphereCollision(m_particles[i].position, m_balls[j].position, m_balls[j].radius + 1.0f))
			{
				m_particles[i].toBeDeleted = true;
			}
		}

		//Check Cloth
		for (size_t j = Config::clothColumns; j < m_cloth.size(); ++j)
		{
			if (Collisions::PointSphereCollision(m_particles[i].position, m_cloth[j].position, m_cloth[j].radius))
			{
				m_particles[i].toBeDeleted = true;
			}
		}

		for (size_t j = 0u; j < m_fans.size(); ++j)
		{
			m_fans[j].InfluenceParticle(m_particles[i]);
		}
	}
	

	for (size_t i = 0u; i < m_balls.size(); ++i)
	{
		//Solids
		for (size_t j = 0u; j < m_solids.size(); ++j)
		{
			if (Collisions::SphereBoxCollision(m_balls[i].position, m_balls[i].radius, m_solids[j].aabb))
			{
				if(Collisions::SphereBoxCollision(m_balls[i].position, m_balls[i].radius, m_solids[j].oobb, contact))
				{
					contact.index = i;
					m_ballReflexions.push_back(contact);
				}
			}
		}

		//Collision Between balls O(N * log(N))
		for (size_t j = i+1; j < m_balls.size(); ++j)
		{
			if (Collisions::SphereSphereCollision(m_balls[i].position, m_balls[i].radius, m_balls[j].position, m_balls[j].radius, contact))
			{
				collision.p1 = &m_balls[i];
				collision.p2 = &m_balls[j];
				collision.contact = contact;
				m_particleCollisions.push_back(collision);
			}
		}

		//Check Cloth
		for (size_t j = Config::clothColumns; j < m_cloth.size(); ++j)
		{
			if (Collisions::SphereSphereCollision(m_balls[i].position, m_balls[i].radius, m_cloth[j].position, m_cloth[j].radius, contact))
			{

				collision.p1 = &m_balls[i];
				collision.p2 = &m_cloth[j];
				collision.contact = contact;
				m_particleCollisions.push_back(collision);
			}
		}

		//Fans
		for (size_t j = 0u; j < m_fans.size(); ++j)
		{
			m_fans[j].InfluenceParticle(m_balls[i]);
		}
	}

	for (size_t i = Config::clothColumns; i < m_cloth.size(); ++i)
	{

		//Solids
		for (size_t j = 0u; j < m_solids.size(); ++j)
		{
			if (Collisions::SphereBoxCollision(m_cloth[i].position, m_cloth[i].radius, m_solids[j].aabb))
			{
				if (Collisions::SphereBoxCollision(m_cloth[i].position, m_cloth[i].radius, m_solids[j].oobb, contact))
				{
					contact.index = i;
					m_clothReflexions.push_back(contact);
				}
			}
		}
		
		//Cloth to cloth
		for (size_t j = i; j < m_cloth.size(); ++j)
		{
			if (Collisions::SphereSphereCollision(m_cloth[i].position, m_cloth[i].radius, m_cloth[j].position, m_cloth[j].radius, contact))
			{
				collision.p1 = &m_cloth[i];
				collision.p2 = &m_cloth[j];
				collision.contact = contact;
				m_particleCollisions.push_back(collision);

			}
		}

		for (size_t j = 0u; j < m_fans.size(); ++j)
		{
			m_fans[j].InfluenceParticle(m_cloth[i]);
		}
	}
}

void ParticleEngine::ApplyForces()
{
	for (size_t i = 0u; i < m_particles.size(); ++i)
	{
		ForceGenerators::ApplyGravity(m_particles[i]);
		ForceGenerators::ApplyAirDrag(m_particles[i]);
	}
	for (size_t i = 0u; i < m_balls.size(); ++i)
	{
		ForceGenerators::ApplyGravity(m_balls[i]);
		ForceGenerators::ApplyAirDrag(m_balls[i]);
	}
	for (size_t i = Config::clothColumns; i < m_cloth.size(); ++i)
	{
		ForceGenerators::ApplyGravity(m_cloth[i]);
		ForceGenerators::ApplyAirDrag(m_cloth[i]);
	}
	for(size_t i = 0u; i < m_springs.size(); ++i)
	{
		ForceGenerators::ApplySpringForces(*(m_springs[i].p1), *(m_springs[i].p2), m_springs[i].restLength, m_springs[i].stiffness);
	}
}

void ParticleEngine::Integrate(float deltaTime)
{
	for (size_t i = 0u; i < m_particles.size(); ++i)
	{
		m_particles[i].Integrate(deltaTime);
	}
	for (size_t i = 0u; i < m_balls.size(); ++i)
	{
		m_balls[i].Integrate(deltaTime);
	}
	for (size_t i = Config::clothColumns; i < m_cloth.size(); ++i)
	{
		m_cloth[i].Integrate(deltaTime);
	}
}

void ParticleEngine::DeleteParticles()
{
	std::vector<Particle>::iterator it = m_particles.begin();
	std::vector<sf::Vertex>::iterator vit = m_particleVertices.begin();

	while(it != m_particles.end())
	{
		if(it->toBeDeleted)
		{
			it = m_particles.erase(it);
			vit = m_particleVertices.erase(vit);
		} else
		{
			++it;
			++vit;
		}
	}	
}

void ParticleEngine::ResolveCollisions()
{

	for (size_t i = 0u; i < m_particleReflexions.size(); ++i)
	{
		ForceGenerators::ApplyReflexion(m_particles[m_particleReflexions[i].index], m_particleReflexions[i]);
	}
	
	for (size_t i = 0u; i < m_ballReflexions.size(); ++i)
	{
		ForceGenerators::ApplyReflexion(m_balls[m_ballReflexions[i].index], m_ballReflexions[i]);
	}

	for (size_t i = 0u; i < m_clothReflexions.size(); ++i)
	{
	ForceGenerators::ApplyReflexion(m_cloth[m_clothReflexions[i].index], m_clothReflexions[i]);
	}	

	for (size_t i = 0u; i < m_particleCollisions.size(); ++i)
	{
		ForceGenerators::ResolveCollision(*m_particleCollisions[i].p1, *m_particleCollisions[i].p2, m_particleCollisions[i].contact);
	}
	m_particleReflexions.clear();
	m_ballReflexions.clear();
	m_clothReflexions.clear();
	m_particleCollisions.clear();
}
