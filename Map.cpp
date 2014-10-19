//Copyright 2011 <>< Charles Lohr - File may be licensed under the MIT/x11 license or the new BSD license.  You choose.

#include "Map.h"
#include "RTHelper.h"
#include <sys/time.h>

Map::Map(string filename, RTHelper * p, bool fake) : /* bQuitBlockUpdater( false ),*/ m_bReloadFullTexture(false), doSubtrace(true), parent(p) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(3, i3DTex);
    for (unsigned i = 0; i < 3; i++) {
        GLTextureData[i] = (RGBA*) malloc(GLH_SIZEX * GLH_SIZEY * GLH_SIZEZ * 4);
        glBindTexture(GL_TEXTURE_3D, i3DTex[i]);
        glTexImage3DEXT(GL_TEXTURE_3D, 0, GL_RGBA8, GLH_SIZEX, GLH_SIZEY, GLH_SIZEZ, 0, GL_RGBA, GL_UNSIGNED_BYTE, GLTextureData[i]);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        glBindTexture(GL_TEXTURE_3D, 0);
    }
    if (fake)
        FakeIt();
    else
        DefaultIt();
    RecalculateAccelerationStructure(0, 0, 0, GLH_SIZEX, GLH_SIZEY, GLH_SIZEZ);
    m_bReloadFullTexture = true;

    printf("Acceleration Structure Calculated.\n");
}

Map::~Map() {
    glDeleteTextures(3, i3DTex);
}

void Map::SetCellInternal(unsigned x, unsigned y, unsigned z, unsigned thiscell, unsigned color) {
    TexCell(0, x, y, z).r = thiscell;
    TexCell(0, x, y, z).g = color;
    TexCell(0, x, y, z).b = parent->LTTex[thiscell * 128 + color * 8 + 7].a * 128.;
    TexCell(0, x, y, z).a = thiscell;
    TexCell(1, x, y, z) = RGBA(0, 0, 0, 0); // (Largescale trace hit?, Jumpmap xy, 


    RGBA & rr = TexCell(1, x, y, z);
    RGBA & rr2 = TexCell(0, x, y, z);
    unsigned char in = rr2.a;
    unsigned char meta = rr.b;
    if (meta > 15) meta = 15;
    TexCell(0, x, y, z).b = parent->LTTex[in * 128 + meta * 8 + 7].a * 128.;

}

void Map::DefaultIt() {

    //All defaults.
    for (unsigned x = 0; x < GLH_SIZEX; x++)
        for (unsigned y = 0; y < GLH_SIZEY; y++)
            for (unsigned z = 0; z < GLH_SIZEZ; z++)
                SetCellInternal(x, y, z, 0, 254);

    for (unsigned x = 0; x < GLH_SIZEX; x++)
        for (unsigned y = 0; y < GLH_SIZEY; y++)
            for (unsigned z = 0; z < GLH_SIZEZ; z++)
                TexCell(2, x, y, z) = RGBA(127, 127, 127, 127);


    //Walls
    for (unsigned x = 0; x < GLH_SIZEX; x++) {
        for (unsigned y = 0; y < GLH_SIZEY; y++) {
            SetCellInternal(x, y, 1, 0xFd, 254);
            SetCellInternal(x, y, GLH_SIZEZ - 2, 0xFd, 254);
        }
    }

    for (unsigned z = 0; z < GLH_SIZEZ; z++)
        for (unsigned x = 0; x < GLH_SIZEX; x++) {
            SetCellInternal(x, 1, z, 0xfd, 254);
            SetCellInternal(x, GLH_SIZEY - 2, z, 0xFd, 254);
        }

    for (unsigned z = 0; z < GLH_SIZEZ; z++)
        for (unsigned y = 0; y < GLH_SIZEY; y++) {
            SetCellInternal(1, y, z, 0xfd, 0xFF);
            SetCellInternal(GLH_SIZEX - 2, y, z, 0xFd, 254);
        }
}

void Map::FakeIt() {
    printf("%p / Fakeit\n", this);


    //World-based boundaries.
    for (unsigned x = 0; x < GLH_SIZEX; x++)
        for (unsigned y = 0; y < GLH_SIZEY; y++)
            for (unsigned z = 0; z < GLH_SIZEZ; z++)
                SetCellInternal(x, y, z, 0, 254);

    for (unsigned x = 0; x < GLH_SIZEX; x++)
        for (unsigned y = 0; y < GLH_SIZEY; y++)
            for (unsigned z = 0; z < GLH_SIZEZ; z++)
                TexCell(2, x, y, z) = RGBA(127, 127, 127, 127);


    for (unsigned x = 0; x < GLH_SIZEX; x++) {
        for (unsigned y = 0; y < GLH_SIZEY; y++) {
            SetCellInternal(x, y, GLH_SIZEZ - 1, 0xFF, 254);
        }
    }

    for (unsigned z = 0; z < GLH_SIZEZ; z++)
        for (unsigned x = 0; x < GLH_SIZEX; x++) {
            SetCellInternal(x, 0, z, 0xff, 254);
            SetCellInternal(x, GLH_SIZEY - 1, z, 0xFF, 254);
        }

    for (unsigned z = 0; z < GLH_SIZEZ; z++)
        for (unsigned y = 0; y < GLH_SIZEY; y++) {
            SetCellInternal(0, y, z, 0xff, 0xFF);
            SetCellInternal(GLH_SIZEX - 1, y, z, 0xFF, 254);
        }

    //For edge portals
    for (unsigned z = 0; z < 7; z++) {
        if (z < 6) {
            SetCellInternal(GLH_SIZEX / 2, 0, z, 0x00, 254);
            SetCellInternal(GLH_SIZEX / 2, GLH_SIZEY - 1, z, 0x00, 254);
        }

        SetCellInternal(GLH_SIZEX / 2 + 1, 0, z, 10, 254);
        SetCellInternal(GLH_SIZEX / 2 + 1, GLH_SIZEY - 1, z, 10, 254);
        SetCellInternal(GLH_SIZEX / 2 - 1, 0, z, 10, 254);
        SetCellInternal(GLH_SIZEX / 2 - 1, GLH_SIZEY - 1, z, 10, 254);

    }

    //Warp Test
    int addpos = parent->AllocAddInfo(5); //For a matrix.

    //	double theta = 1.6;
    //	parent->AdditionalInformationMapData[addpos+0] = RGBAf( cos(theta), -sin(theta), 0, 0 );
    //	parent->AdditionalInformationMapData[addpos+1] = RGBAf( sin(theta),  cos(theta), 0, 0 );

    //Rotation?  Super Jankey
    //	parent->AdditionalInformationMapData[addpos+0] = RGBAf( 0, -1, 0, 0 );
    //	parent->AdditionalInformationMapData[addpos+1] = RGBAf( 1, 0, 0, 0 );

    parent->AdditionalInformationMapData[addpos + 0] = RGBAf(1, 0, 0, 0);
    parent->AdditionalInformationMapData[addpos + 1] = RGBAf(0, 1, 0, 0);
    parent->AdditionalInformationMapData[addpos + 2] = RGBAf(0, 0, 1, 0);
    parent->AdditionalInformationMapData[addpos + 3] = RGBAf(21, 30, 0, 0);

    printf("ADDPOS: %d\n", addpos);
    for (unsigned z = 0; z < 4; z++) {
        for (unsigned x = 0; x < 6; x++)
            for (unsigned y = 0; y < 6; y++) {
                TexCell(1, x + 10, y + 10, z) = RGBA(0, 0, addpos % ADDSIZEX, addpos / ADDSIZEX);
            }
    }
    parent->MarkAddInfoForReload();

    //Ground
    for (unsigned x = 0; x < GLH_SIZEX; x++) {
        for (unsigned y = 0; y < GLH_SIZEY; y++) {
            SetCellInternal(x, y, 1, 2, 254);
        }
    }

    //left side of unusual wall
    for (unsigned y = 22; y < 43; y++) {
        int x = 36;
        SetCellInternal(x, y + 10, 2, rand() % 15, 254);
        SetCellInternal(x, y + 10, 3, rand() % 15, 254);
        SetCellInternal(x, y + 10, 4, rand() % 15, 254);
    }


    for (unsigned y = 10; y < 40; y += 10) {
        for (unsigned x = 20; x < 35; x++) {
            if (y == 10 || y == 20) {
                //The physical bridges
                SetCellInternal(x, y + 0, 2, 15, 254);
                SetCellInternal(x, y + 0, 3, 15, 254);
                SetCellInternal(x, y + 0, 4, 15, 254);
                SetCellInternal(x, y + 0, 5, 16, 254);
                SetCellInternal(x, y + 1, 5, 15, 254);
                SetCellInternal(x, y + 2, 5, 15, 254);
                SetCellInternal(x, y + 3, 5, 15, 254);
                SetCellInternal(x, y + 3, 4, 15, 254);
                SetCellInternal(x, y + 3, 3, 15, 254);
                SetCellInternal(x, y + 3, 2, 15, 254);
            } else {
                SetCellInternal(x, y + 10, 2, rand() % 15, 254);
                SetCellInternal(x, y + 10, 3, rand() % 15, 254);
                SetCellInternal(x, y + 10, 4, rand() % 15, 254);
            }

            RGBA cond;

            if (y == 10) {
                cond = RGBA(127, 30, 30, 30);
            } else if (y == 20) {
                cond = RGBA(20, 127, 127, 127);
            } else if (y == 30) {
                cond = RGBA(255, 30, 30, 30);
            }

            if (x > 20) //Do not set the low-end of any of the squares. (Except Z)
            {
                TexCell(2, x, y + 1, 2) = cond;
                TexCell(2, x, y + 1, 3) = cond;
                TexCell(2, x, y + 1, 4) = cond;
                TexCell(2, x, y + 1, 5) = cond;
                TexCell(2, x, y + 2, 2) = cond;
                TexCell(2, x, y + 2, 3) = cond;
                TexCell(2, x, y + 2, 4) = cond;
                TexCell(2, x, y + 2, 5) = cond;
                TexCell(2, x, y + 3, 2) = cond;
                TexCell(2, x, y + 3, 3) = cond;
                TexCell(2, x, y + 3, 4) = cond;
                TexCell(2, x, y + 3, 5) = cond;
            }
        }
    }


    //Make a oversized room at 40, 40
    for (int y = 40; y < 45; y++)
        for (int x = 20; x < 25; x++)
            for (int z = 2; z < 4; z++) {
                if (y != 40 && x != 20 && z < 4)
                    TexCell(2, x, y, z) = RGBA(9, 9, 9, 100);
                if (y == 42 && (x == 20 || x == 24)) {
                } else if (y == 40 || y == 44 || x == 20 || x == 24 || z == 3)
                    SetCellInternal(x, y, z, 4, 254);

            }

    for (int y = 42; y < 44; y++)
        for (int z = 2; z < 4; z++) {
            TexCell(2, 20, y, z) = RGBA(9, 9, 9, 20);
            TexCell(2, 25, y, z) = RGBA(9, 9, 9, 20);
        }

    //for( int i = 0; i < 10; i++ )
    //	SetCellInternal( rand()%5 + 20, rand()%5+40, 1, 10, 254 );
    parent->MapOffsetX = 0;
    parent->MapOffsetY = 0;
    parent->MapOffsetZ = 20;

    m_bTriggerFullRecalculate = true;
}


unsigned char skipTypes[23] = {06, 20, 37, 38, 39, 40, 51, 55, 59, 63, 64, 65, 66, 68, 69, 71, 75, 76, 77, 83, 85, 86, 91};

void Map::RecalculateAccelerationStructure(int ix, int iy, int iz, int sx, int sy, int sz) {
    //	printf( "Recalculate %d %d %d    %d %d %d\n", ix, iy, iz, sx, sy, sz );
    //increasing X
    //	int minval;

    //OLD: 44.804s
    //NEW: 3.306s
    //Then: 1.4s
    //	printf( "BENCHMARK IN\n" );
    //	timeval tv1;
    //	gettimeofday( &tv1, 0 );	

    unsigned char inskips[GLH_SIZEX * GLH_SIZEY * GLH_SIZEZ];
    //	unsigned char ouskips[GLH_SIZEX*GLH_SIZEY*GLH_SIZEZ];
    if (parent->LTTex != NULL) {
        for (int z = iz; z < sz; z++)
            for (int y = iy; y < sy; y++)
                for (int x = ix; x < sx; x++) {
                    //RGBA & rr = TexCell( 1, x, y, z );
                    RGBA & rr2 = TexCell(0, x, y, z);
                    unsigned char in = rr2.a;
                    inskips[x + y * GLH_SIZEX + z * GLH_SIZEX * GLH_SIZEY] = in != 0;
                    //XXX do skip processing here
                    for (unsigned int i = 0; i < 23; ++i)
                        if (in == skipTypes[i])
                            inskips[x + y * GLH_SIZEX + z * GLH_SIZEX * GLH_SIZEY] = 0;

                    //			if( meta > 15 ) meta = 15;
                    //			TexCell( 0, x, y, z ).b = parent->LTTex[in * 128 + meta * 8 + 7].a*128.;
                }
    } else {
        printf("FATAL ERROR: LTTex was not populated.\n");
    }


    //Needs to be unsigned here, because we may % negative numbers.
    if (doSubtrace) {
        for (unsigned z = iz; z < sz; z++)
            for (unsigned y = iy; y < sy; y++)
                for (unsigned x = ix; x < sx; x++) {

                    unsigned char in = inskips[x + y * GLH_SIZEX + z * GLH_SIZEX * GLH_SIZEY]; //<0,0,0>
                    unsigned char in1 = inskips[((x - 1) % GLH_SIZEX) + y * GLH_SIZEX + z * GLH_SIZEX * GLH_SIZEY]; //<-1,0,0>
                    unsigned char in2 = inskips[x + ((y - 1) % GLH_SIZEY) * GLH_SIZEX + z * GLH_SIZEX * GLH_SIZEY]; //<0,-1,0>
                    unsigned char in3 = inskips[x + y * GLH_SIZEX + ((z - 1) % GLH_SIZEZ) * GLH_SIZEX * GLH_SIZEY]; //<0,0,-1>
                    unsigned char in4 = inskips[((x - 1) % GLH_SIZEX)+((y - 1) % GLH_SIZEY) * GLH_SIZEX + z * GLH_SIZEX * GLH_SIZEY]; //<-1,-1,0>
                    unsigned char in5 = inskips[((x - 1) % GLH_SIZEX) + y * GLH_SIZEX + ((z - 1) % GLH_SIZEZ) * GLH_SIZEX * GLH_SIZEY]; //<-1,0,-1>
                    unsigned char in6 = inskips[x + ((y - 1) % GLH_SIZEY) * GLH_SIZEX + ((z - 1) % GLH_SIZEZ) * GLH_SIZEX * GLH_SIZEY]; //<0,-1,-1>
                    unsigned char in7 = inskips[((x - 1) % GLH_SIZEX)+((y - 1) % GLH_SIZEY) * GLH_SIZEX + ((z - 1) % GLH_SIZEZ) * GLH_SIZEX * GLH_SIZEY]; //<-1,-1,-1>

                    unsigned char cc = (in || in1 || in2 || in3 || in4 || in5 || in6 || in7) ? 0xFF : 0;

                    TexCell(1, x, y, z).r = cc;
                    //ouskips[x+y*GLH_SIZEX+z*GLH_SIZEX*GLH_SIZEY] = (cc)?1:MAX_SEEKER;

                }
    } else {
        fprintf(stderr, "We are not doing thigns without subtracing anymore.\n");
    }

}

void Map::UpdateCellSpecific(int x, int y, int z, int sx, int sy, int sz) {
    RecalculateAccelerationStructure(x - 1, y - 1, z - 1, x + 1 + sx, y + 1 + sy, z + 1 + sz);
}

void Map::Draw() {

    list<CellUpdate> liu;

    while (!ListUpdates.empty()) {
        CellUpdate f = ListUpdates.front();
        ListUpdates.pop_front();
        if (f.sx * f.sy * f.sz == 0) continue;
        UpdateCellSpecific(f.x, f.y, f.z, f.sx, f.sy, f.sz);
        liu.push_back(f);
    }

    glActiveTextureARB(GL_TEXTURE0);
    for (unsigned i = 0; i < 3; i++) {
        glBindTexture(GL_TEXTURE_3D, i3DTex[i]);
        for (list< CellUpdate >::iterator li = liu.begin(); li != liu.end(); li++) {
            int sx = li->sx + 2;
            int sy = li->sy + 2;
            int sz = li->sz + 2;
            int ix = li->x;
            int iy = li->y;
            int iz = li->z; //any update can effect us and the cell to our upper left.
            RGBA buffer[sx * sy * sz];

            int lx, ly, lz;
            int stridea = sx*sy;
            int strideb = sx;
            for (lz = 0; lz < sz; lz++)
                for (ly = 0; ly < sy; ly++)
                    for (lx = 0; lx < sx; lx++)
                        buffer[lz * stridea + ly * strideb + lx] = TexCell(i, ix + lx - 1, iy + ly - 1, iz + lz - 1);

            glTexSubImage3DEXT(GL_TEXTURE_3D, 0, ix - 1, iy - 1, iz - 1, sx, sy, sz, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
        }
        glFinish();
    }
    glBindTexture(GL_TEXTURE_3D, 0);


    if (m_bReloadFullTexture) {
        printf("Reloading texture...\n");
        glActiveTextureARB(GL_TEXTURE0);
        for (unsigned i = 0; i < 3; i++) {
            glBindTexture(GL_TEXTURE_3D, i3DTex[i]);
            glTexSubImage3DEXT(GL_TEXTURE_3D, 0, 0, 0, 0, GLH_SIZEX, GLH_SIZEY, GLH_SIZEZ, GL_RGBA, GL_UNSIGNED_BYTE, GLTextureData[i]);
            glFinish();
        }
        glBindTexture(GL_TEXTURE_3D, 0);

        m_bReloadFullTexture = false;
    }


    glEnable(GL_TEXTURE_3D);

    glActiveTextureARB(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, i3DTex[0]);
    glActiveTextureARB(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, i3DTex[1]);
    glActiveTextureARB(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_3D, i3DTex[2]);
}