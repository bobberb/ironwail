/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
Copyright (C) 2010-2014 QuakeSpasm developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"

#define MAX_PARTICLES			16384	// default max # of particles at one
										//  time
#define ABSOLUTE_MIN_PARTICLES	512		// no fewer than this no matter what's
										//  on the command line

// Quake color ramps
static int	ramp1[8] = {0x6f, 0x6d, 0x6b, 0x69, 0x67, 0x65, 0x63, 0x61};
static int	ramp2[8] = {0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x68, 0x66};
static int	ramp3[8] = {0x6d, 0x6b, 6, 5, 4, 3};

// H2: Additional color ramps for Hexen II particle effects
static int ramp4[16] = { 416, 416+1, 416+2, 416+3, 416+4, 416+5, 416+6, 416+7, 416+8, 416+9, 416+10, 416+11, 416+12, 416+13, 416+14, 416+15 };
static int ramp5[16] = { 400, 400+1, 400+2, 400+3, 400+4, 400+5, 400+6, 400+7, 400+8, 400+9, 400+10, 400+11, 400+12, 400+13, 400+14, 400+15 };
static int ramp6[16] = { 256, 256+1, 256+2, 256+3, 256+4, 256+5, 256+6, 256+7, 256+8, 256+9, 256+10, 256+11, 256+12, 256+13, 256+14, 256+15 };
static int ramp7[16] = { 384, 384+1, 384+2, 384+3, 384+4, 384+5, 384+6, 384+7, 384+8, 384+9, 384+10, 384+11, 384+12, 384+13, 384+14, 384+15 };
static int ramp8[16] = { 175, 174, 173, 172, 171, 170, 169, 168, 167, 166, 13, 14, 15, 16, 17, 18 };
static int ramp9[16] = { 416, 416+1, 416+2, 416+3, 416+4, 416+5, 416+6, 416+7, 416+8, 416+9, 416+10, 416+11, 416+12, 416+13, 416+14, 416+15 };
static int ramp10[16] = { 432, 432+1, 432+2, 432+3, 432+4, 432+5, 432+6, 432+7, 432+8, 432+9, 432+10, 432+11, 432+12, 432+13, 432+14, 432+15 };
static int ramp11[8] = { 424, 424+1, 424+2, 424+3, 424+4, 424+5, 424+6, 424+7 };
static int ramp12[8] = { 136, 137, 138, 139, 140, 141, 142, 143 };

particle_t	*particles;
int			r_numparticles, r_numactiveparticles;

static float uvscale;
static float texturescalefactor; //johnfitz -- compensate for apparent size of different particle textures

cvar_t	r_particles = {"r_particles","2", CVAR_ARCHIVE}; //johnfitz

typedef struct particlevert_t {
	vec3_t		pos;
	GLubyte		color[4];
} particlevert_t;

static particlevert_t partverts[MAX_PARTICLES];
static int numpartverts = 0;

/*
===============
R_SetParticleTexture_f -- johnfitz
===============
*/
static void R_SetParticleTexture_f (cvar_t *var)
{
	switch ((int)(r_particles.value))
	{
	case 1:
		uvscale = 1;
		texturescalefactor = 1.27;
		break;
	case 2:
		uvscale = 0.25;
		texturescalefactor = 1.0;
		break;
	}
}

/*
===============
R_AllocParticle
===============
*/
particle_t *R_AllocParticle (void)
{
	if (r_numactiveparticles < r_numparticles)
	{
		particle_t *p = &particles[r_numactiveparticles++];
		p->spawn = cl.time - 0.001;
		return p;
	}
	return NULL;
}

/*
===============
R_InitParticles
===============
*/
void R_InitParticles (void)
{
	int		i;

	i = COM_CheckParm ("-particles");

	if (i && i < com_argc - 1)
	{
		r_numparticles = atoi(com_argv[i + 1]);
		if (r_numparticles < ABSOLUTE_MIN_PARTICLES)
			r_numparticles = ABSOLUTE_MIN_PARTICLES;
	}
	else
	{
		r_numparticles = MAX_PARTICLES;
	}

	particles = (particle_t *)
			Hunk_AllocName (r_numparticles * sizeof(particle_t), "particles");
	r_numactiveparticles = 0;

	Cvar_RegisterVariable (&r_particles); //johnfitz
	Cvar_SetCallback (&r_particles, R_SetParticleTexture_f);
	R_SetParticleTexture_f (&r_particles); // set default
}

/*
===============
R_EntityParticles
===============
*/
static vec3_t	avelocities[NUMVERTEXNORMALS];
static float	beamlength = 16;

void R_EntityParticles (entity_t *ent)
{
	int		i;
	particle_t	*p;
	float		angle;
	float		sp, sy, cp, cy;
//	float		sr, cr;
//	int		count;
	vec3_t		forward;
	float		dist;

	dist = 64;
//	count = 50;

	if (!avelocities[0][0])
	{
		for (i = 0; i < NUMVERTEXNORMALS; i++)
		{
			avelocities[i][0] = (rand() & 255) * 0.01;
			avelocities[i][1] = (rand() & 255) * 0.01;
			avelocities[i][2] = (rand() & 255) * 0.01;
		}
	}

	for (i = 0; i < NUMVERTEXNORMALS; i++)
	{
		angle = cl.time * avelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = cl.time * avelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);
		angle = cl.time * avelocities[i][2];
	//	sr = sin(angle);
	//	cr = cos(angle);

		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;

		if (!(p = R_AllocParticle ()))
			return;

		p->die = cl.time + 0.01;
		p->color = 0x6f;
		p->type = pt_explode;

		p->org[0] = ent->origin[0] + r_avertexnormals[i][0]*dist + forward[0]*beamlength;
		p->org[1] = ent->origin[1] + r_avertexnormals[i][1]*dist + forward[1]*beamlength;
		p->org[2] = ent->origin[2] + r_avertexnormals[i][2]*dist + forward[2]*beamlength;
	}
}

/*
===============
R_ClearParticles
===============
*/
void R_ClearParticles (void)
{
	r_numactiveparticles = 0;
}

/*
===============
R_ParseParticleEffect

Parse an effect out of the server message
===============
*/
void R_ParseParticleEffect (void)
{
	vec3_t		org, dir;
	int			i, count, msgcount, color;

	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord (cl.protocolflags);
	for (i=0 ; i<3 ; i++)
		dir[i] = MSG_ReadChar () * (1.0/16);
	msgcount = MSG_ReadByte ();
	color = MSG_ReadByte ();

	if (msgcount == 255)
		count = 1024;
	else
		count = msgcount;

	R_RunParticleEffect (org, dir, color, count);
}

/*
===============
R_ParticleExplosion
===============
*/
void R_ParticleExplosion (vec3_t org)
{
	int			i, j;
	particle_t	*p;

	for (i=0 ; i<1024 ; i++)
	{
		if (!(p = R_AllocParticle ()))
			return;

		p->die = cl.time + 5;
		p->color = ramp1[0];
		p->ramp = rand()&3;
		if (i & 1)
		{
			p->type = pt_explode;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (rand()%512)-256;
			}
		}
		else
		{
			p->type = pt_explode2;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (rand()%512)-256;
			}
		}
	}
}

/*
===============
R_ParticleExplosion2
===============
*/
void R_ParticleExplosion2 (vec3_t org, int colorStart, int colorLength)
{
	int			i, j;
	particle_t	*p;
	int			colorMod = 0;

	for (i=0; i<512; i++)
	{
		if (!(p = R_AllocParticle ()))
			return;

		p->die = cl.time + 0.3;
		p->color = colorStart + (colorMod % colorLength);
		colorMod++;

		p->type = pt_blob;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()%32)-16);
			p->vel[j] = (rand()%512)-256;
		}
	}
}

/*
===============
R_BlobExplosion
===============
*/
void R_BlobExplosion (vec3_t org)
{
	int			i, j;
	particle_t	*p;

	for (i=0 ; i<1024 ; i++)
	{
		if (!(p = R_AllocParticle ()))
			return;

		p->die = cl.time + 1 + (rand()&8)*0.05;

		if (i & 1)
		{
			p->type = pt_blob;
			p->color = 66 + rand()%6;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (rand()%512)-256;
			}
		}
		else
		{
			p->type = pt_blob2;
			p->color = 150 + rand()%6;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (rand()%512)-256;
			}
		}
	}
}

/*
===============
R_RunParticleEffect
===============
*/
void R_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count)
{
	int			i, j;
	particle_t	*p;

	for (i=0 ; i<count ; i++)
	{
		if (!(p = R_AllocParticle ()))
			return;

		if (count == 1024)
		{	// rocket explosion
			p->die = cl.time + 5;
			p->color = ramp1[0];
			p->ramp = rand()&3;
			if (i & 1)
			{
				p->type = pt_explode;
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = org[j] + ((rand()%32)-16);
					p->vel[j] = (rand()%512)-256;
				}
			}
			else
			{
				p->type = pt_explode2;
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = org[j] + ((rand()%32)-16);
					p->vel[j] = (rand()%512)-256;
				}
			}
		}
		else
		{
			p->die = cl.time + 0.1*(rand()%5);
			p->color = (color&~7) + (rand()&7);
			p->type = pt_slowgrav;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()&15)-8);
				p->vel[j] = dir[j]*15;// + (rand()%300)-150;
			}
		}
	}
}

/*
===============
R_LavaSplash
===============
*/
void R_LavaSplash (vec3_t org)
{
	int			i, j, k;
	particle_t	*p;
	float		vel;
	vec3_t		dir;

	for (i=-16 ; i<16 ; i++)
		for (j=-16 ; j<16 ; j++)
			for (k=0 ; k<1 ; k++)
			{
				if (!(p = R_AllocParticle ()))
					return;

				p->die = cl.time + 2 + (rand()&31) * 0.02;
				p->color = 224 + (rand()&7);
				p->type = pt_slowgrav;

				dir[0] = j*8 + (rand()&7);
				dir[1] = i*8 + (rand()&7);
				dir[2] = 256;

				p->org[0] = org[0] + dir[0];
				p->org[1] = org[1] + dir[1];
				p->org[2] = org[2] + (rand()&63);

				VectorNormalize (dir);
				vel = 50 + (rand()&63);
				VectorScale (dir, vel, p->vel);
			}
}

/*
===============
R_TeleportSplash
===============
*/
void R_TeleportSplash (vec3_t org)
{
	int			i, j, k;
	particle_t	*p;
	float		vel;
	vec3_t		dir;

	for (i=-16 ; i<16 ; i+=4)
	{
		for (j=-16 ; j<16 ; j+=4)
		{
			for (k=-24 ; k<32 ; k+=4)
			{
				if (!(p = R_AllocParticle ()))
					return;

				p->die = cl.time + 0.2 + (rand()&7) * 0.02;
				p->color = 7 + (rand()&7);
				p->type = pt_slowgrav;

				dir[0] = j*8;
				dir[1] = i*8;
				dir[2] = k*8;

				p->org[0] = org[0] + i + (rand()&3);
				p->org[1] = org[1] + j + (rand()&3);
				p->org[2] = org[2] + k + (rand()&3);

				VectorNormalize (dir);
				vel = 50 + (rand()&63);
				VectorScale (dir, vel, p->vel);
			}
		}
	}
}

/*
===============
R_RainEffect

H2: Create rain particles within a bounding box
===============
*/
void R_RainEffect (vec3_t org, vec3_t e_size, int x_dir, int y_dir, int color, int count)
{
	int			i, holdint;
	particle_t	*p;
	float		z_time;

	for (i = 0; i < count; i++)
	{
		if (!(p = R_AllocParticle ()))
			return;

		p->vel[0] = x_dir;
		p->vel[1] = y_dir;
		p->vel[2] = -(rand() % 956);
		if (p->vel[2] > -256)
			p->vel[2] += -256;

		z_time = -(e_size[2] / p->vel[2]);
		p->die = cl.time + z_time;
		p->color = color;
		p->ramp = rand() & 3;
		p->type = pt_rain;

		holdint = (int)e_size[0];
		p->org[0] = org[0] + (holdint ? (rand() % holdint) : 0);
		holdint = (int)e_size[1];
		p->org[1] = org[1] + (holdint ? (rand() % holdint) : 0);
		p->org[2] = org[2];
	}
}

/*
===============
R_SnowEffect

H2: Create snow particles within a bounding box with SFL_ flags
===============
*/
void R_SnowEffect (vec3_t org1, vec3_t org2, int flags, vec3_t alldir, int count)
{
	int			i, holdint;
	particle_t	*p;

	for (i = 0; i < count; i++)
	{
		if (!(p = R_AllocParticle ()))
			return;

		p->vel[0] = alldir[0];
		p->vel[1] = alldir[1];
		p->vel[2] = alldir[2] * ((rand() & 15) + 7) / 10.0f;

		p->flags = flags;

		// Determine flake size based on flags
		if (flags & H2_SFL_FLUFFY || (flags & H2_SFL_MIXED && (rand() & 3)))
			p->count = (rand() & 31) + 10;
		else
			p->count = 10;

		// Determine color based on brightness flag
		if (flags & H2_SFL_HALF_BRIGHT)
			p->color = 26 + (rand() % 5);
		else
			p->color = 18 + (rand() % 12);

		// Add translucency if not disabled
		if (!(flags & H2_SFL_NO_TRANS))
			p->color += 256;

		p->die = cl.time + 7;
		p->ramp = rand() & 3;
		p->type = pt_snow;

		holdint = (int)(org2[0] - org1[0]);
		p->org[0] = org1[0] + (holdint ? (rand() % holdint) : 0);
		holdint = (int)(org2[1] - org1[1]);
		p->org[1] = org1[1] + (holdint ? (rand() % holdint) : 0);
		p->org[2] = org2[2];

		VectorCopy (org1, p->min_org);
		VectorCopy (org2, p->max_org);
	}
}

/*
===============
R_ColoredParticleExplosion

H2: Create explosion with custom color
===============
*/
void R_ColoredParticleExplosion (vec3_t org, int color, int radius, int counter)
{
	int			i, j;
	particle_t	*p;

	for (i = 0; i < counter; i++)
	{
		if (!(p = R_AllocParticle ()))
			return;

		p->die = cl.time + 3;
		p->color = color;
		p->ramp = rand() & 3;

		if (i & 1)
		{
			p->type = pt_c_explode;
			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + ((rand() % (radius * 2)) - radius);
				p->vel[j] = (rand() & 511) - 256;
			}
		}
		else
		{
			p->type = pt_c_explode2;
			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + ((rand() % (radius * 2)) - radius);
				p->vel[j] = (rand() & 511) - 256;
			}
		}
	}
}

/*
===============
R_RunQuakeEffect

H2: Earthquake particle effect
===============
*/
void R_RunQuakeEffect (vec3_t org, float distance)
{
	int			i;
	particle_t	*p;
	float		num, num2;
	float		s, c;

	for (i = 0; i < 100; i++)
	{
		if (!(p = R_AllocParticle ()))
			return;

		p->die = cl.time + 0.3f * (rand() % 5);
		p->color = (rand() & 3) + ((rand() % 3) * 16) + (13 * 16) + 256 + 11;
		p->type = pt_quake;
		p->ramp = 0;

		num = rand() * (1.0f / RAND_MAX);
		num2 = distance * num;
		num = rand() * (1.0f / RAND_MAX);
		s = sinf(num * 2 * M_PI);
		c = cosf(num * 2 * M_PI);
		p->org[0] = org[0] + c * num2;
		p->org[1] = org[1] + s * num2;
		p->org[2] = org[2];

		num = rand() * (1.0f / RAND_MAX);
		p->vel[0] = (num * 40) - 20;
		num = rand() * (1.0f / RAND_MAX);
		p->vel[1] = (num * 40) - 20;
		num = rand() * (1.0f / RAND_MAX);
		p->vel[2] = 65 * num + 80;
	}
}

/*
===============
R_RunParticleEffect2

H2: Extended particle effect with velocity bounds
===============
*/
void R_RunParticleEffect2 (vec3_t org, vec3_t dmin, vec3_t dmax, int color, ptype_t effect, int count)
{
	int			i, j;
	particle_t	*p;
	float		num;

	for (i = 0; i < count; i++)
	{
		if (!(p = R_AllocParticle ()))
			return;

		p->die = cl.time + 2;
		p->color = color;
		p->type = effect;
		p->ramp = 0;
		for (j = 0; j < 3; j++)
		{
			num = rand() * (1.0f / RAND_MAX);
			p->org[j] = org[j] + ((rand() & 8) - 4);
			p->vel[j] = dmin[j] + ((dmax[j] - dmin[j]) * num);
		}
	}
}

/*
===============
R_RunParticleEffect3

H2: Extended particle effect with box bounds
===============
*/
void R_RunParticleEffect3 (vec3_t org, vec3_t box, int color, ptype_t effect, int count)
{
	int			i, j;
	particle_t	*p;
	float		num;

	for (i = 0; i < count; i++)
	{
		if (!(p = R_AllocParticle ()))
			return;

		p->die = cl.time + 2;
		p->color = color;
		p->type = effect;
		p->ramp = 0;
		for (j = 0; j < 3; j++)
		{
			num = rand() * (1.0f / RAND_MAX);
			p->org[j] = org[j] + ((rand() & 15) - 8);
			p->vel[j] = (box[j] * num * 2) - box[j];
		}
	}
}

/*
===============
R_RunParticleEffect4

H2: Extended particle effect with radius
===============
*/
void R_RunParticleEffect4 (vec3_t org, float radius, int color, ptype_t effect, int count)
{
	int			i, j;
	particle_t	*p;
	float		num;

	for (i = 0; i < count; i++)
	{
		if (!(p = R_AllocParticle ()))
			return;

		p->die = cl.time + 2;
		p->color = color;
		p->type = effect;
		p->ramp = 0;
		for (j = 0; j < 3; j++)
		{
			num = rand() * (1.0f / RAND_MAX);
			p->org[j] = org[j] + ((rand() & 15) - 8);
			p->vel[j] = (radius * num * 2) - radius;
		}
	}
}

/*
===============
R_SunStaffTrail

H2: Sun staff weapon trail
===============
*/
void R_SunStaffTrail (vec3_t source, vec3_t dest)
{
	int			i;
	particle_t	*p;
	vec3_t		vec, dist;
	float		length, size;

	VectorSubtract (dest, source, vec);
	length = VectorNormalize (vec);
	VectorCopy (vec, dist);

	size = 10;

	while (length > 0)
	{
		length -= size;

		if (!(p = R_AllocParticle ()))
			return;

		p->die = cl.time + 2;
		p->ramp = rand() & 3;
		p->color = ramp6[(int)p->ramp];
		p->type = pt_spit;

		for (i = 0; i < 3; i++)
			p->org[i] = source[i] + ((rand() & 3) - 2);

		p->vel[0] = (rand() % 10) - 5;
		p->vel[1] = (rand() % 10) - 5;
		p->vel[2] = (rand() % 10);

		VectorAdd (source, dist, source);
	}
}

/*
===============
R_RocketTrail

Supports both Quake trail types (0-6) and Hexen II trail types (rt_* enum values)
===============
*/
void R_RocketTrail (vec3_t start, vec3_t end, int type)
{
	vec3_t		vec, dist;
	float		len, size, lifetime;
	int			j;
	particle_t	*p;
	int			dec;
	static int	tracercount;

	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	// Default values
	size = 3;
	lifetime = 2;
	VectorScale (vec, 3, dist);

	// Handle Quake's dense trail modifier
	if (type < 128 && type >= 0 && type <= 6)
	{
		dec = 3;
	}
	else if (type >= 128 && type < 128 + 7)
	{
		dec = 1;
		type -= 128;
	}
	else
	{
		// H2 trail types - set size based on type
		switch (type)
		{
		case rt_spit:
			size = 1;
			break;
		case rt_ice:
			size = 15;
			VectorScale (vec, 15, dist);
			break;
		case rt_acidball:
			size = 5;
			lifetime = 0.8f;
			break;
		default:
			size = 3;
			VectorScale (vec, 3, dist);
			break;
		}
		dec = (int)size;
	}

	while (len > 0)
	{
		len -= (dec > 0) ? dec : size;

		if (!(p = R_AllocParticle ()))
			return;

		VectorCopy (vec3_origin, p->vel);
		p->die = cl.time + lifetime;

		switch (type)
		{
			case rt_rocket_trail:	// rocket trail (0)
				p->ramp = (rand()&3);
				p->color = ramp3[(int)p->ramp];
				p->type = pt_fire;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()%6)-3);
				break;

			case rt_smoke:	// smoke (1)
				p->ramp = (rand()&3) + 2;
				p->color = ramp3[(int)p->ramp];
				p->type = pt_fire;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()%6)-3);
				break;

			case rt_blood:	// blood (2)
				p->type = pt_slowgrav;
				p->color = 134 + (rand()&7);
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()%6)-3);
				break;

			case rt_tracer:
			case rt_tracer2:	// tracer (3, 5)
				p->die = cl.time + 0.5;
				p->type = pt_static;
				if (type == rt_tracer)
					p->color = 130 + (rand()&6);
				else
					p->color = 230 + ((tracercount&4)<<1);

				tracercount++;

				VectorCopy (start, p->org);
				if (tracercount & 1)
				{
					p->vel[0] = 30*vec[1];
					p->vel[1] = 30*-vec[0];
				}
				else
				{
					p->vel[0] = 30*-vec[1];
					p->vel[1] = 30*vec[0];
				}
				break;

			case rt_slight_blood:	// slight blood (4)
				p->type = pt_slowgrav;
				p->color = 134 + (rand()&7);
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()%6)-3);
				len -= size;
				break;

			case rt_voor_trail:	// voor trail (6)
				p->color = 9*16 + 8 + (rand()&3);
				p->type = pt_static;
				p->die = cl.time + 0.3;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&15)-8);
				break;

			// H2: Fireball trail
			case rt_fireball:
				p->ramp = rand() & 3;
				p->color = ramp4[(int)p->ramp];
				p->type = pt_fireball;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&3)-2);
				p->org[2] += 2;	// compensate for model
				p->vel[0] = (rand() % 200) - 100;
				p->vel[1] = (rand() % 200) - 100;
				p->vel[2] = (rand() % 200) - 100;
				break;

			// H2: Ice trail
			case rt_ice:
				p->ramp = rand() & 3;
				p->color = ramp5[(int)p->ramp];
				p->type = pt_ice;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&3)-2);
				p->org[2] += 2;
				p->vel[0] = (rand() % 16) - 8;
				p->vel[1] = (rand() % 16) - 8;
				p->vel[2] = (rand() % 20) - 40;
				break;

			// H2: Spit trail
			case rt_spit:
				p->ramp = rand() & 3;
				p->color = ramp6[(int)p->ramp];
				p->type = pt_spit;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&3)-2);
				p->org[2] += 2;
				p->vel[0] = (rand() % 10) - 5;
				p->vel[1] = (rand() % 10) - 5;
				p->vel[2] = (rand() % 10);
				break;

			// H2: Spell trail
			case rt_spell:
				p->ramp = rand() & 3;
				p->color = ramp6[(int)p->ramp];
				p->type = pt_spell;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&3)-2);
				p->vel[0] = vec[0] * -10;
				p->vel[1] = vec[1] * -10;
				p->vel[2] = vec[2] * -10;
				break;

			// H2: Vorpal missile trail
			case rt_vorpal:
				p->type = pt_vorpal;
				p->color = 44 + (rand() & 3) + 256;
				for (j=0 ; j<2 ; j++)
					p->org[j] = start[j] + ((rand() % 48) - 24);
				p->org[2] = start[2] + ((rand() & 15) - 8);
				break;

			// H2: Set staff trail
			case rt_setstaff:
				p->type = pt_setstaff;
				p->color = ramp9[0];
				p->ramp = rand() & 3;
				for (j=0 ; j<2 ; j++)
					p->org[j] = start[j] + ((rand() % 6) - 3);
				p->org[2] = start[2] + ((rand() % 10) - 5);
				p->vel[0] = (rand() & 7) - 4;
				p->vel[1] = (rand() & 7) - 4;
				break;

			// H2: Magic missile trail
			case rt_magicmissile:
				p->type = pt_magicmissile;
				p->color = 148 + (rand() & 11);
				p->ramp = rand() & 3;
				for (j=0 ; j<2 ; j++)
					p->org[j] = start[j] + ((rand() % 48) - 24);
				p->org[2] = start[2] + ((rand() % 48) - 24);
				p->vel[2] = -((rand() & 15) + 8);
				break;

			// H2: Bone shard trail
			case rt_boneshard:
				p->type = pt_boneshard;
				p->color = 368 + (rand() & 16);
				for (j=0 ; j<2 ; j++)
					p->org[j] = start[j] + ((rand() % 48) - 24);
				p->org[2] = start[2] + ((rand() % 48) - 24);
				p->vel[2] = -((rand() & 15) + 8);
				break;

			// H2: Scarab staff trail
			case rt_scarab:
				p->type = pt_scarab;
				p->color = 250 + (rand() & 3);
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + (rand() & 7);
				p->vel[2] = -(rand() & 7);
				break;

			// H2: Acid ball trail
			case rt_acidball:
				p->ramp = rand() & 3;
				p->color = ramp10[(int)p->ramp];
				p->type = pt_acidball;
				p->die = cl.time + 0.5;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&3)-2);
				p->org[2] += 2;
				p->vel[0] = (rand() % 40) - 20;
				p->vel[1] = (rand() % 40) - 20;
				p->vel[2] = (rand() % 40) - 20;
				break;

			// H2: Bloodshot trail
			case rt_bloodshot:
				p->type = pt_darken;
				p->color = 136 + (rand() & 5);
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand() & 3) - 2);
				len -= size;
				break;
		}

		VectorAdd (start, dist, start);
	}
}

/*
===============
CL_RunParticles -- johnfitz -- all the particle behavior, separated from R_DrawParticles
===============
*/
void CL_RunParticles (void)
{
	particle_t		*p;
	int				i, cur, active;
	float			time1, time2, time3, dvel, frametime, grav;
	extern	cvar_t	sv_gravity;

	frametime = cl.time - cl.oldtime;
	time3 = frametime * 15;
	time2 = frametime * 10;
	time1 = frametime * 5;
	grav = frametime * sv_gravity.value * 0.05;
	dvel = 4*frametime;

	for (cur = active = 0, p = particles; cur < r_numactiveparticles; cur++, p++)
	{
		if (p->die < cl.time || p->spawn > cl.time)
			continue;

		p->org[0] += p->vel[0]*frametime;
		p->org[1] += p->vel[1]*frametime;
		p->org[2] += p->vel[2]*frametime;

		switch (p->type)
		{
		case pt_static:
			break;
		case pt_fire:
			p->ramp += time1;
			if (p->ramp >= 6)
				p->die = -1;
			else
				p->color = ramp3[(int)p->ramp];
			p->vel[2] += grav;
			break;

		case pt_explode:
			p->ramp += time2;
			if (p->ramp >=8)
				p->die = -1;
			else
				p->color = ramp1[(int)p->ramp];
			for (i=0 ; i<3 ; i++)
				p->vel[i] += p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_explode2:
			p->ramp += time3;
			if (p->ramp >=8)
				p->die = -1;
			else
				p->color = ramp2[(int)p->ramp];
			for (i=0 ; i<3 ; i++)
				p->vel[i] -= p->vel[i]*frametime;
			p->vel[2] -= grav;
			break;

		case pt_blob:
			for (i=0 ; i<3 ; i++)
				p->vel[i] += p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_blob2:
			for (i=0 ; i<2 ; i++)
				p->vel[i] -= p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_grav:
		case pt_slowgrav:
			p->vel[2] -= grav;
			break;

		// H2: Additional particle types
		case pt_fastgrav:
			p->vel[2] -= grav * 4;
			break;

		case pt_rain:
			// Rain handled specially - no physics update needed
			break;

		case pt_snow:
			// Snow handled specially - no physics update needed here
			break;

		case pt_c_explode:
			p->ramp += time2;
			if ((int)p->ramp >= 8)
				p->die = -1;
			else if (time2)
				p->color--;
			for (i=0 ; i<3 ; i++)
				p->vel[i] += p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_c_explode2:
			p->ramp += time3;
			if ((int)p->ramp >= 8)
				p->die = -1;
			else if (time3)
				p->color -= 2;
			for (i=0 ; i<3 ; i++)
				p->vel[i] -= p->vel[i]*frametime;
			p->vel[2] -= grav;
			break;

		case pt_fireball:
			p->ramp += time3;
			if ((int)p->ramp >= 16)
				p->die = -1;
			else
				p->color = ramp4[(int)p->ramp];
			break;

		case pt_acidball:
			p->ramp += time3 * 1.4f;
			if ((int)p->ramp >= 23)
				p->die = -1;
			else if ((int)p->ramp >= 15)
				p->color = ramp11[(int)p->ramp - 15];
			else
				p->color = ramp10[(int)p->ramp];
			p->vel[2] -= grav;
			break;

		case pt_spit:
			p->ramp += time3;
			if ((int)p->ramp >= 16)
				p->die = -1;
			else
				p->color = ramp6[(int)p->ramp];
			break;

		case pt_ice:
			p->ramp += time3 * 1.33f;
			if ((int)p->ramp >= 16)
				p->die = -1;
			else
				p->color = ramp5[(int)p->ramp];
			p->vel[2] -= grav;
			break;

		case pt_spell:
			p->ramp += time2;
			if ((int)p->ramp >= 16)
				p->die = -1;
			else
				p->color = ramp7[(int)p->ramp];
			break;

		case pt_test:
			p->vel[2] += 1.3f;
			p->ramp += time3;
			if ((int)p->ramp >= 13 || ((int)p->ramp > 10 && (int)p->vel[2] < 20))
				p->die = -1;
			else
				p->color = ramp8[(int)p->ramp];
			break;

		case pt_quake:
			p->vel[0] *= 1.05f;
			p->vel[1] *= 1.05f;
			p->vel[2] -= grav * 4;
			break;

		case pt_vorpal:
			--p->color;
			if ((int)p->color <= 37 + 256)
				p->die = -1;
			break;

		case pt_setstaff:
			p->ramp += time1;
			if ((int)p->ramp >= 16)
				p->die = -1;
			else
				p->color = ramp9[(int)p->ramp];
			p->vel[0] *= 1.08f;
			p->vel[1] *= 1.08f;
			p->vel[2] -= grav * 0.5f;
			break;

		case pt_magicmissile:
			--p->color;
			if ((int)p->color < 149)
				p->color = 149;
			p->ramp += time1;
			if ((int)p->ramp > 16)
				p->die = -1;
			break;

		case pt_boneshard:
			--p->color;
			if ((int)p->color < 368)
				p->die = -1;
			break;

		case pt_scarab:
			--p->color;
			if ((int)p->color < 250)
				p->die = -1;
			break;

		case pt_redfire:
			p->ramp += frametime * 3;
			if ((int)p->ramp >= 8)
				p->die = -1;
			else
				p->color = ramp12[(int)p->ramp] + 256;
			p->vel[0] *= 0.9f;
			p->vel[1] *= 0.9f;
			p->vel[2] += grav * 0.5f;
			break;

		case pt_darken:
			p->vel[2] -= grav;
			--p->color;
			// Check if color reached darkest point in its range
			{
				int colindex = 0;
				while (colindex < 224)
				{
					if (colindex == 192 || colindex == 200)
						colindex += 8;
					else
						colindex += 16;
					if (p->color == colindex)
						p->die = -1;
				}
			}
			break;

		case pt_gravwell:
		case pt_rd:
			// Gravity well/Rider death - particles spiral toward origin
			// Note: Full implementation requires tracking target origin
			p->vel[2] -= grav * 0.3f;
			break;
		}

		if (cur != active)
			particles[active] = *p;
		active++;
	}

	r_numactiveparticles = active;
}

/*
===============
R_FlushParticleBatch
===============
*/
static void R_FlushParticleBatch (void)
{
	GLuint buf;
	GLbyte *ofs;

	if (!numpartverts)
		return;

	GL_Upload (GL_ARRAY_BUFFER, partverts, sizeof(partverts[0]) * numpartverts, &buf, &ofs);
	GL_BindBuffer (GL_ARRAY_BUFFER, buf);
	GL_VertexAttribPointerFunc (0, 3, GL_FLOAT, GL_FALSE, sizeof(partverts[0]), ofs + offsetof(particlevert_t, pos));
	GL_VertexAttribPointerFunc (1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(partverts[0]), ofs + offsetof(particlevert_t, color));

	GL_DrawArraysInstancedFunc (GL_TRIANGLE_STRIP, 0, 4, numpartverts);

	numpartverts = 0;
}

/*
===============
R_DrawParticles_Real -- johnfitz -- moved all non-drawing code to CL_RunParticles
===============
*/
static void R_DrawParticles_Real (qboolean alpha, qboolean showtris)
{
	particle_t		*p;
	particlevert_t	*v;
	GLubyte			color[4] = {255, 255, 255, 255}, *c; //johnfitz -- particle transparency
	extern	cvar_t	r_particles; //johnfitz
	//float			alpha; //johnfitz -- particle transparency
	float			scalex, scaley;
	qboolean		dither, oit;
	int				i;

	if (!r_particles.value)
		return;

	if (!r_numactiveparticles)
		return;

	// square particles are drawn opaque (avoiding alpha sorting issues)
	if (!showtris && alpha != ((int)r_particles.value != 2))
		return;

	GL_BeginGroup ("Particles");

	dither = (softemu == SOFTEMU_COARSE && !showtris);
	oit = (alpha && R_GetEffectiveAlphaMode () == ALPHAMODE_OIT);
	GL_UseProgram (glprogs.particles[oit][dither]);

	// compensate for apparent size of different particle textures
	// this bakes in the additional scaling of vup and vright by 1.5f for billboarding,
	// then down by 0.25f for quad particles
	scalex = scaley = texturescalefactor * 0.375f;
	// projection factors (see GL_FrustumMatrix), negated to make things easier in the shader
	scalex *=  r_matproj[1*4 + 0]; // -1 / tan (fovx/2)
	scaley *= -r_matproj[2*4 + 1]; // -1 / tan (fovy/2)
	GL_Uniform3fFunc (0, scalex, scaley, uvscale);

	if (alpha)
		GL_SetState (GLS_BLEND_ALPHA_OIT | GLS_NO_ZWRITE | GLS_CULL_NONE | GLS_ATTRIBS (2) | GLS_INSTANCED_ATTRIBS (2));
	else
		GL_SetState (GLS_BLEND_OPAQUE | GLS_CULL_NONE | GLS_ATTRIBS (2) | GLS_INSTANCED_ATTRIBS (2));

	numpartverts = 0;
	for (i = 0, p = particles; i < r_numactiveparticles; i++, p++)
	{
		if (numpartverts == countof(partverts))
			R_FlushParticleBatch ();

		v = &partverts[numpartverts++];
		VectorCopy (p->org, v->pos);

		//johnfitz -- particle transparency and fade out
		c = showtris ? color : (GLubyte *) &d_8to24table[(int)p->color];
		*(uint32_t*)&v->color = *(uint32_t*)c;
		v->color[0] = c[0];
		v->color[1] = c[1];
		v->color[2] = c[2];
		//alpha = CLAMP(0, p->die + 0.5 - cl.time, 1);
		v->color[3] = c[3]; //(int)(alpha * 255);
		//johnfitz
	}

	R_FlushParticleBatch ();

	GL_EndGroup ();
}

/*
===============
R_DrawParticles -- johnfitz -- moved all non-drawing code to CL_RunParticles
===============
*/
void R_DrawParticles (qboolean alpha)
{
	R_DrawParticles_Real (alpha, false);
}
/*
===============
R_DrawParticles_ShowTris -- johnfitz
===============
*/
void R_DrawParticles_ShowTris (void)
{
	R_DrawParticles_Real (false, true);
}

