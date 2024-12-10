################################################################################################################
## Dragon Age End User Toolset
##
## Lightmap processing script
##
## Author: James Goldman
## Date: March 18, 2009
##
## Copyright (c) 2009 BioWare Corp.
################################################################################################################

import win32com
import struct
import pywintypes
import os.path
import ctypes
import aergia
import xml.dom.minidom
from win32com.client import constants
import sys
import getopt
import subprocess
import glob
import shutil

GAMMA = 1.5

def MakeFourCC(str):
    if len(str) != 4:
        return 0
    return ord(str[3]) << 24 | ord(str[2]) << 16 | ord(str[1]) << 8 | ord(str[0])

def MatrixMultiply(lhs,rhs):
    """Multiply two 4x4 matrices in the form of lists of 16 floats"""
    if len(lhs) != len(rhs) or len(lhs) != 16:
        print "Need 4x4 matrices"
        return None

    res = [0 for x in range(0, 16)]
    for i in range(0,4):
        for k in range(0,4):
            for j in range(0,4):
                res[i*4+k]+=lhs[i*4+j]*rhs[j*4+k]
    return res

class GFFID:

    GFF_NAME                                = 2

    GFF_MMH_NAME                            = 6000
    GFF_MODEL_HIERARCHY_MODEL_DATA_NAME     = 6005
    GFF_MMH_MESH_GROUP_NAME                 = 6006
    GFF_MMH_TRANSLATION                     = 6047
    GFF_MMH_ROTATION                        = 6048
    GFF_MMH_MESH_CAST_BAKED_SHADOW          = 6177
    GFF_MMH_MESH_RECEIVE_BAKED_SHADOW       = 6301
    GFF_MMH_CHILDREN                        = 6999
    
    GFF_MESH_CHUNK_VERTEXSIZE               = 8000
    GFF_MESH_CHUNK_VERTEXCOUNT              = 8001
    GFF_MESH_CHUNK_INDEXCOUNT               = 8002
    GFF_MESH_CHUNK_PRIMITIVETYPE            = 8003
    GFF_MESH_CHUNK_INDEXFORMAT              = 8004
    GFF_MESH_CHUNK_VERTEXOFFSET             = 8006
    GFF_MESH_CHUNK_STARTINDEX               = 8009
    GFF_MESH_BOUNDS_SPHERE                  = 8019
    GFF_MESH_CHUNK_BOUNDS                   = 8020
    GFF_MESH_CHUNKS                         = 8021
    GFF_MESH_VERTEXDATA                     = 8022
    GFF_MESH_INDEXDATA                      = 8023
    GFF_MESH_CHUNK_VERTEXDECLARATOR         = 8025
    GFF_MESH_VERTEXDECLARATOR_OFFSET        = 8027
    GFF_MESH_VERTEXDECLARATOR_DATATYPE      = 8028
    GFF_MESH_VERTEXDECLARATOR_USAGE         = 8029
    GFF_MESH_VERTEXDECLARATOR_USAGEINDEX    = 8030

class MSHChunk:

    # Vertex data types
    TYPE_FLOAT1     = 0
    TYPE_FLOAT2     = 1
    TYPE_FLOAT3     = 2
    TYPE_FLOAT4     = 3
    TYPE_COLOR      = 4
    TYPE_UBYTE4     = 5
    TYPE_SHORT2     = 6
    TYPE_SHORT4     = 7
    TYPE_UBYTE4N    = 8
    TYPE_SHORT2N    = 9
    TYPE_SHORT4N    = 10
    TYPE_USHORT2N   = 11
    TYPE_USHORT4N   = 12
    TYPE_UDEC3      = 13
    TYPE_DEC3N      = 14
    TYPE_FLOAT16_2  = 15
    TYPE_FLOAT16_4  = 16

    # These are the only vertex usages we care about for lightmapping
    USAGE_POSITION  = 0
    USAGE_NORMAL    = 3
    USAGE_TEXCOORD  = 5

    # Index formats
    IF_16  = 0
    IF_32 = 1

    # Primitive types
    PT_TRIANGLE = 0

    def Float16To32(self, num):
        """Convert half-precision float to full float"""
        z = ctypes.c_long((num & 0x8000) << 16) # Sign bit
        z.value = z.value | (num & 0x03ff) << 13 # Position mantissa
        z.value = z.value | (((num & 0x7C00) >> 10) + 0x70) << 23 # Rebias and position exponent
        return struct.unpack_from("f", z)[0]

    def ParseVertex(self, VertexData, offset, format):
        """Parse the data at the given offset using the given format into a list of floats"""

        if format == MSHChunk.TYPE_FLOAT1:
            return struct.unpack_from("f", VertexData, offset)
        elif format == MSHChunk.TYPE_FLOAT2:
            return struct.unpack_from("ff", VertexData, offset)
        elif format == MSHChunk.TYPE_FLOAT3:
            return struct.unpack_from("fff", VertexData, offset)
        elif format == MSHChunk.TYPE_FLOAT4:
            return struct.unpack_from("ffff", VertexData, offset)
        elif format == MSHChunk.TYPE_UBYTE4:
            return float(ord(VertexData[offset]))
        elif format == MSHChunk.TYPE_UBYTE4N:
            return float(ord(VertexData[offset])) / 255.0
        elif format == MSHChunk.TYPE_SHORT2:
            return map(float, struct.unpack_from("hh", VertexData, offset))
        elif format == MSHChunk.TYPE_SHORT4:
            return map(float, struct.unpack_from("hhhh", VertexData, offset))
        elif format == MSHChunk.TYPE_SHORT2N:
            return map(lambda x: float(x) / 65535.0, struct.unpack_from("hh", VertexData, offset))
        elif format == MSHChunk.TYPE_SHORT4N:
            return map(lambda x: float(x) / 65535.0, struct.unpack_from("hhhh", VertexData, offset))
        elif format == MSHChunk.TYPE_UDEC3:
            x = struct.unpack_from("I", VertexData, offset)[0]
            return [float(x & 0x3ff), float(x >> 10 & 0x3ff), float(x >> 20 & 0x3ff)]
        elif format == MSHChunk.TYPE_DEC3N:
            x = struct.unpack_from("I", VertexData, offset)[0]
            return [float(x & 0x3ff)/511.0, float(x >> 10 & 0x3ff)/511.0, float(x >> 20 & 0x3ff)/511.0]
        elif format == MSHChunk.TYPE_FLOAT16_2:
            return map(self.Float16To32, struct.unpack_from("HH", VertexData, offset))
        elif format == MSHChunk.TYPE_FLOAT16_4:
            return map(self.Float16To32, struct.unpack_from("HHHH", VertexData, offset))

    def __init__(self, reader, VertexData, IndexData):

        if reader.GetValueByLabel(GFFID.GFF_MESH_CHUNK_PRIMITIVETYPE) != MSHChunk.PT_TRIANGLE:
            print "Primitive types other than triangle not currently supported."
            return

        self.name = reader.GetValueByLabel(GFFID.GFF_NAME)
        vertexSize = reader.GetValueByLabel(GFFID.GFF_MESH_CHUNK_VERTEXSIZE)
        vertexCount = reader.GetValueByLabel(GFFID.GFF_MESH_CHUNK_VERTEXCOUNT)
        chunkOffset = reader.GetValueByLabel(GFFID.GFF_MESH_CHUNK_VERTEXOFFSET)

        # Bounding sphere for light culling
        self.bsphere = reader.GetValueByLabel(GFFID.GFF_MESH_CHUNK_BOUNDS).GetValueByLabel(GFFID.GFF_MESH_BOUNDS_SPHERE)

        declarators = reader.GetValueByLabel(GFFID.GFF_MESH_CHUNK_VERTEXDECLARATOR)

        # Position declaration
        self.positions = []
        rdr = [x for x in declarators if x.GetValueByLabel(GFFID.GFF_MESH_VERTEXDECLARATOR_USAGE) == MSHChunk.USAGE_POSITION][0]
        posOffset = rdr.GetValueByLabel(GFFID.GFF_MESH_VERTEXDECLARATOR_OFFSET)
        posType = rdr.GetValueByLabel(GFFID.GFF_MESH_VERTEXDECLARATOR_DATATYPE)

        # Normal declaration
        # TODO: Normals are not currently getting to Yafray. Lightmaps would look much better with them, though
        self.normals = []
        normalType = None
        try:
            rdr = [x for x in declarators if x.GetValueByLabel(GFFID.GFF_MESH_VERTEXDECLARATOR_USAGE) == MSHChunk.USAGE_NORMAL][0]
            normalOffset = rdr.GetValueByLabel(GFFID.GFF_MESH_VERTEXDECLARATOR_OFFSET)
            normalType = rdr.GetValueByLabel(GFFID.GFF_MESH_VERTEXDECLARATOR_DATATYPE)
        except IndexError:
            pass # No normal data found - occluding geometry?

        # Texture coordinate declaration
        self.texcoords = []
        texcoordType = None
        try:
            # Try to get a texture coordinate declarator in channel one, where most models have their UV
            # data. If that doesn't exist, it might be terrain so try channel zero. After that, give up.
            tcdecl = [x for x in declarators if x.GetValueByLabel(GFFID.GFF_MESH_VERTEXDECLARATOR_USAGE) == MSHChunk.USAGE_TEXCOORD]
            try:
                rdr = [x for x in tcdecl if x.GetValueByLabel(GFFID.GFF_MESH_VERTEXDECLARATOR_USAGEINDEX) == 1][0]
            except IndexError:
                rdr = [x for x in tcdecl if x.GetValueByLabel(GFFID.GFF_MESH_VERTEXDECLARATOR_USAGEINDEX) == 0][0]

            texcoordOffset = rdr.GetValueByLabel(GFFID.GFF_MESH_VERTEXDECLARATOR_OFFSET)
            texcoordType = rdr.GetValueByLabel(GFFID.GFF_MESH_VERTEXDECLARATOR_DATATYPE)
            texcoordUsage = rdr.GetValueByLabel(GFFID.GFF_MESH_VERTEXDECLARATOR_USAGEINDEX)
        except (IndexError, pywintypes.com_error):
            pass # No texture coordinate data found - occluding geometry?

        # Now we can actually parse the BLOB into vertices
        for i in range(0, vertexCount):
            baseOffset = chunkOffset + i * vertexSize
            self.positions += self.ParseVertex(VertexData, baseOffset + posOffset, posType)[0:3] # Drop W element, if it exists
            if texcoordType != None:
                vert = self.ParseVertex(VertexData, baseOffset + texcoordOffset, texcoordType)
                vert[1] = 1.0 - vert[1] # Yafray seems to use a different coordinate space than our tools
                self.texcoords += vert
            if normalType != None:
                self.normals += self.ParseVertex(VertexData, baseOffset + normalOffset, normalType)

        # Read in the indices.
        indexFormat = reader.GetValueByLabel(GFFID.GFF_MESH_CHUNK_INDEXFORMAT)
        indexCount = reader.GetValueByLabel(GFFID.GFF_MESH_CHUNK_INDEXCOUNT)
        indexStart = reader.GetValueByLabel(GFFID.GFF_MESH_CHUNK_STARTINDEX)
        fmt = "h" if indexFormat == MSHChunk.IF_16 else "i"
        self.indices = map(int, struct.unpack_from(fmt*indexCount, IndexData, indexStart*struct.calcsize(fmt)))

    def CreateAergiaMesh(self, transform, material, scene):
        vertexbuffer = aergia.buffer(aergia.BufferUsage_Position, len(self.positions))
        vertexbuffer.setData(self.positions)
        vertexcount = len(self.positions)/3

        UVcount = len(self.texcoords)/2
        texcoordbuffer = aergia.buffer(aergia.BufferUsage_Texcoord, len(self.texcoords))
        if UVcount > 0:
            texcoordbuffer.setData(self.texcoords)

        indexbuffer = aergia.buffer(aergia.BufferUsage_Index, len(self.indices))
        indexbuffer.setData(self.indices)
        tricount = len(self.indices)/3

        return aergia.mesh(self.name, vertexbuffer, 0, vertexcount,
            texcoordbuffer, 0, UVcount,
            indexbuffer, 0, tricount,
            transform, self.bsphere, scene, material)

class GFFMSHReader:

    def __init__(self, filename):

        self.chunks = {}

        try:
            self.reader = win32com.client.gencache.EnsureDispatch("Bioware.Eclipse.EngGFFReader")
            self.reader.LoadGFFFromFile(filename)
        except pywintypes.com_error:
            print "Could not load form %s" % (filename)
            return

        if self.reader.MagicNumber != constants.EngGFFMagicNumber or self.reader.GFFVersion != constants.EngGFFVersion40:
            print "Error: %s is not a GFF 4.0 file" % (filename)
            return

        try:

            # Get the shared raw data
            VertexData = self.reader.TopLevelStruct.GetValueByLabel(GFFID.GFF_MESH_VERTEXDATA)
            IndexData = self.reader.TopLevelStruct.GetValueByLabel(GFFID.GFF_MESH_INDEXDATA)

            # Create the chunks
            for reader in self.reader.TopLevelStruct.GetValueByLabel(GFFID.GFF_MESH_CHUNKS):
                chunk = MSHChunk(reader, VertexData, IndexData)
                self.chunks[chunk.name] = chunk

        except pywintypes.com_error:
            print "Error parsing %s" % (filename)

def GetMMHTransform(reader):
    """Read the transform out of the child nodes of the given GFF structure."""

    t = (0, 0, 0, 1)
    r = (0, 0, 0, 1)

    for child in [ref.GetTargetValue() for ref in reader.GetValueByLabel(GFFID.GFF_MMH_CHILDREN)]:
        if child.StructType == MakeFourCC("trsl"):
            t = child.GetValueByLabel(GFFID.GFF_MMH_TRANSLATION)
        elif child.StructType == MakeFourCC("rota"):
            r = child.GetValueByLabel(GFFID.GFF_MMH_ROTATION)

    return [1-2*(r[1]*r[1]+r[2]*r[2]), 2*(r[0]*r[1]-r[3]*r[2]), 2*(r[0]*r[2]+r[3]*r[1]), t[0],
        2*(r[0]*r[1]+r[3]*r[2]), 1-2*(r[0]*r[0]+r[2]*r[2]), 2*(r[1]*r[2]-r[3]*r[0]), t[1],
        2*(r[0]*r[2]-r[3]*r[1]), 2*(r[1]*r[2]+r[3]*r[0]), 1-2*(r[0]*r[0]+r[1]*r[1]), t[2],
        0, 0, 0, t[3] ]

class MMHPart:

    def __init__(self, structReader, transform):

        # Note: It appears that our binarizer will not create sub-parts, so no need to propagate the transform any further
        self.receive_baked_shadow = False
        self.cast_baked_shadow = False
        self.transform = MatrixMultiply(transform, GetMMHTransform(structReader))
        self.name = ""

        try:
            self.name = structReader.GetValueByLabel(GFFID.GFF_MMH_NAME)
            self.meshgroup = structReader.GetValueByLabel(GFFID.GFF_MMH_MESH_GROUP_NAME)
            self.receive_baked_shadow = structReader.GetValueByLabel(GFFID.GFF_MMH_MESH_RECEIVE_BAKED_SHADOW) != 0
            self.cast_baked_shadow = structReader.GetValueByLabel(GFFID.GFF_MMH_MESH_CAST_BAKED_SHADOW) != 0
        except pywintypes.com_error:
            pass

class GFFMMHReader:

    def __init__(self, filename):

        self.meshreader = None
        self.parts = []

        """Read in and parse an MMH and its associated meshes."""
        try:
            # EnsureDispatch generates the wrapper class if necessary
            reader = win32com.client.gencache.EnsureDispatch("Bioware.Eclipse.EngGFFReader")
        except pywintypes.com_error:
            print "Could not get a GFF reader object. Make sure Engine.dll is registered."
            return

        try:
            reader.LoadGFFFromFile(filename)
        except pywintypes.com_error:
            print "Could not load from %s" % (filename)
            return

        if reader.MagicNumber != constants.EngGFFMagicNumber or reader.GFFVersion != constants.EngGFFVersion40:
            print "Error: %s is not a GFF 4.0 file" % (filename)
            return

        try:

            # Load the mesh file
            meshpath = os.path.join(os.path.dirname(filename), reader.TopLevelStruct.GetValueByLabel(GFFID.GFF_MODEL_HIERARCHY_MODEL_DATA_NAME))
            if not os.path.exists(meshpath):
                print "Error: mesh file referenced in %s does not exist in the directory." % (filename)
                return
            self.meshreader = GFFMSHReader(meshpath)

            # Get the root node. Make sure there's only one
            roots = [ref.GetTargetValue() for ref in reader.TopLevelStruct.GetValueByLabel(GFFID.GFF_MMH_CHILDREN)]
            if len(roots) != 1:
                print "Error: model in %s does not have exactly one root node" % (filename)
                return

            # Get the transform for the whole works
            transform = GetMMHTransform(roots[0])

            # The root should have mesh nodes as its direct children
            parts = [ ref.GetTargetValue() for ref in roots[0].GetValueByLabel(GFFID.GFF_MMH_CHILDREN) if ref.GetTargetValue().StructType == MakeFourCC("mshh")]
            if len(parts) == 0:
                print "Error: model in %s contains no parts." % (filename)
                return

            for part in parts:
                self.parts.append(MMHPart(part, transform))

        except pywintypes.com_error:
            print "Error parsing %s" % (filename)

    def CreateMeshes(self, transform, material, scene):
        """Create and add the meshes to the scene, and return a map of chunk names --> mesh instances."""    
        chunks = {}
        for part in self.parts:
            chunks[part.name] = self.meshreader.chunks[part.meshgroup].CreateAergiaMesh(
                aergia.geometry.matrix4x4(MatrixMultiply(transform, part.transform)), material, scene)
        return chunks

def BuildScene(scenefile, instances):
    """Parse the scene xml and build the scene in aergia"""
    material = aergia.materials.shinydiffuse( 'whitemat', 
        aergia.geometry.vector3d(1.0, 1.0, 1.0), 0.0, 0.0, 1.0, 0.0, 0.0, 1.3, 0.0 )
    scene = aergia.scene()
    xmldoc = xml.dom.minidom.parse(scenefile)
    for object in xmldoc.getElementsByTagName("Object"):
        mmhreader = GFFMMHReader(object.getAttribute("Source"))
        for instance in object.getElementsByTagName("Instance"):
            instanceName = instance.getAttribute("Name")
            transform = map(lambda x: float(instance.getElementsByTagName("Transform")[0].getAttribute(x)), 
                ["M00","M10","M20","M30","M01","M11","M21","M31","M02","M12","M22","M32","M03","M13","M23","M33"])
            instances[instanceName] = mmhreader.CreateMeshes(transform, material, scene)
    return scene

def ParseLight(xmlnode, forLightmap):
    type = xmlnode.getAttribute("Type").lower()
    castShadows = True if xmlnode.getAttribute("CastShadows") == "true" else False
    
    brightness = float(xmlnode.getElementsByTagName("Brightness")[0].childNodes[0].nodeValue)
    colour = map(lambda x: float(xmlnode.getElementsByTagName("Colour")[0].getAttribute(x)), ("R", "G", "B"))
    transform = aergia.geometry.matrix4x4(map(lambda x: float(xmlnode.getElementsByTagName("Transform")[0].getAttribute(x)), 
            ["M00","M10","M20","M30","M01","M11","M21","M31","M02","M12","M22","M32","M03","M13","M23","M33"]))

    # If this is for the shadowmaps, check to see if it's marked as casting shadows
    if not forLightmap and not castShadows:
        brightness = 0.0
        colour = [0.0, 0.0, 0.0]

    GUIDNode = xmlnode.getElementsByTagName("GUID")[0]
    GUID = GUIDNode.getAttribute("A") + GUIDNode.getAttribute("B") + GUIDNode.getAttribute("C") + GUIDNode.getAttribute("D")

    radius = 0.0
    innerAngle = 0.0
    outerAngle = 30.0
    try:
        radius = float(xmlnode.getElementsByTagName("Radius")[0].childNodes[0].nodeValue)
        innerAngle = float(xmlnode.getElementsByTagName("InnerRadius")[0].childNodes[0].nodeValue) * 180.0 / 3.141592
        outerAngle = float(xmlnode.getElementsByTagName("OuterRadius")[0].childNodes[0].nodeValue) * 180.0 / 3.141592
    except IndexError:
        pass

    col = aergia.geometry.vector3d(colour[0], colour[1], colour[2])
    position = aergia.geometry.vector3d(0.0, 0.0, 0.0)
    position.transform(transform)

    if type == "point":
        return GUID, aergia.lights.point(GUID, position, col, brightness, radius)
    elif type == "ambient":
        # TODO: brightness for ambients
        return GUID, aergia.lights.ambient(GUID, col)
        pass
    elif type == "directional":
        return GUID, aergia.lights.directional(GUID, col, brightness)
    elif type == "spot":
        # TODO: radius for spotlights not supported by Yafray?
        falloff = 1.0
        if outerAngle > 0:
            falloff = 1.0 - (innerAngle/outerAngle)
        to = aergia.geometry.vector3d(0.0,-1.0,0.0)
        to.transform(transform)
        return GUID, aergia.lights.spot(GUID, col, outerAngle, position, to, brightness, falloff, radius)

def ProcessLightmapJob(scene, instances, jobfile):
    """Process a single lightmap job file."""

    print "Processing lightmap job %s ..." % (jobfile)
    sys.stdout.flush()

    MAXRAYDEPTH = 6
    scene.addObject(aergia.integrators.directlight('integator', MAXRAYDEPTH, 0, 0, 0))

    # Get the lights
    xmldoc = xml.dom.minidom.parse(jobfile)
    for lightnode in xmldoc.getElementsByTagName("Light"):
        light = ParseLight(lightnode, True)
        if light != None:
            scene.addObject(light[1])

    # Get the render targets (i.e. the objects to be lightmapped)
    for target in xmldoc.getElementsByTagName("RenderTarget"):
        modelkey = target.getAttribute("Model")
        modelpart = target.getAttribute("Part")
        width = int(target.getAttribute("SizeX"))
        height = int(target.getAttribute("SizeY"))
        outfile = target.getAttribute("OutputFile")

        try:
            if not terse:
                print "Processing %s" % (os.path.basename(outfile))
                sys.stdout.flush()
            instance = instances[modelkey]
            mesh = instance[modelpart]
            film = aergia.film(outfile, width, height, aergia.FilmFilterType_Gauss, 1, GAMMA, 1, 0)
            camera = aergia.lightmapcam(film, mesh)
            scene.render(film, camera)
        except KeyError:
            if not terse:
                print "Failed to find part %s on %s" % (modelpart, modelkey)

def ProcessShadowmapJob(scene, instances, jobfile):
    """Process a single shadowmap job file."""
    MAXRAYDEPTH = 6
    scene.addObject(aergia.integrators.directlight('integator', MAXRAYDEPTH, 0, 0, 1))

    print "Processing shadow map job %s ..." % (jobfile)
    sys.stdout.flush()

    # Get the lights
    xmldoc = xml.dom.minidom.parse(jobfile)
    for lightnode in xmldoc.getElementsByTagName("Light"):
        light = ParseLight(lightnode, False)
        if light != None:
            scene.addObject(light[1])

    # Get the render targets (i.e. the objects to be lightmapped)
    for target in xmldoc.getElementsByTagName("RenderTarget"):
        modelkey = target.getAttribute("Model")
        modelpart = target.getAttribute("Part")
        width = int(target.getAttribute("SizeX"))
        height = int(target.getAttribute("SizeY"))
        outfile = target.getAttribute("OutputFile")

        try:
            if not terse:
                print "Processing %s" % (os.path.basename(outfile))
                sys.stdout.flush()
            instance = instances[modelkey]
            mesh = instance[modelpart]
            film = aergia.film(outfile, width, height, aergia.FilmFilterType_Gauss, 1, 1, 1, 0)
            camera = aergia.lightmapcam(film, mesh)
            scene.render(film, camera)
        except KeyError:
            if not terse:
                print "Failed to find part %s on %s" % (modelpart, modelkey)

def ProcessAmbientOcclusionJob(scene, instances, jobfile):
    """Process a single ambient occlusion job file."""

    # TODO: read the ray length and number of samples from the ao job file
    raylength = 5
    aorays = 5

    scene.addObject(aergia.integrators.ambientocclusion(
        'AOIntegrator', aorays, raylength, aergia.geometry.vector3d(1.0, 1.0, 1.0)))

    print "Processing ambient occlusion job %s ..." % (jobfile)
    sys.stdout.flush()

    xmldoc = xml.dom.minidom.parse(jobfile)
    # Get the render targets (i.e. the objects to be lightmapped)
    for target in xmldoc.getElementsByTagName("RenderTarget"):
        modelkey = target.getAttribute("Model")
        modelpart = target.getAttribute("Part")
        width = int(target.getAttribute("SizeX"))
        height = int(target.getAttribute("SizeY"))
        outfile = target.getAttribute("OutputFile")
        try:
            if not terse:
                print "Processing %s" % (os.path.basename(outfile))
                sys.stdout.flush()
            instance = instances[modelkey]
            mesh = instance[modelpart]
            film = aergia.film(outfile, width, height, aergia.FilmFilterType_Gauss, 1, GAMMA, 1, 0)
            camera = aergia.lightmapcam(film, mesh)
            scene.render(film, camera)
        except KeyError:
            if not terse:
                print "Failed to find part %s on %s" % (modelpart, modelkey)

def ProcessJobs(folder, processor, basename):
    """Process all numbered jobs found in the given folder which match the given template."""
    i = 0
    while True:
        jobfile = os.path.join(folder, basename % (i))
        if not os.path.exists(jobfile): break
        instances = {}
        # Create the scene in the raytracer environment
        scene = BuildScene(os.path.join(folder, "job_scene.xml"), instances)
        processor(scene, instances, jobfile)
        i = i + 1
        aergia.ClearAll()

def ProcessFolder(folder):
    """Processes one folder containing lightmapping and/or shadowmapping and/or ambient occlusion jobs."""

    ProcessJobs(folder, ProcessLightmapJob, "job_lm%d.xml")
    ProcessJobs(folder, ProcessAmbientOcclusionJob, "job_ao%d.xml")
    ProcessJobs(folder, ProcessShadowmapJob, "job_sm%d.xml")

def DefaultDir(dir, default):
    """Test to see if the directory name is specified. If not, create the given default directory."""
    if len(dir) == 0:
        dir = default
        if not os.path.exists(dir):
            os.mkdir(dir)
    return os.path.normpath(dir)

def Execute(command):
    startupinfo = subprocess.STARTUPINFO()
    startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    p = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, startupinfo = startupinfo)
    outtext, errtext = p.communicate()
    if len(outtext) != 0:
        print outtext
    if p.returncode == 0:
        return True
    return False

def main():
    FAIL = 1
    SUCCESS = 0

    options = 'i:n:o:c:m:e:v'
    longOptions = ['inputDir=',
                   'outputTGADir=',
                   'outputAoTGADir=',
                   'combinedDDSDir=',
                   'compressedDDSDir=',
                   'atlasedDDSDir=',
                   'numSubJobs=',       # Not actually used, but maybe should be
                   'atlasFile=',
                   'in_width=',
                   'in_height=',
                   'notify_complete',
                   'terse',
                   'cpus=']             # This is here to avoid the GetoptError. It's actually parsed by eclipseray :S

    try:
        opts, pargs = getopt.getopt(sys.argv, options, longOptions)
    except getopt.GetoptError:
        print "Lightmapper.py error: Invalid input parameter specified."
        return FAIL

    global terse

    inputDir = ""
    numSubJobs = 1
    outputTGADir = ""
    outputAoTGADir = ""
    combinedDDSDir = ""
    atlasedDDSDir = ""
    compressedDDSDir = ""
    atlasFile = ""
    in_width = "512"
    in_height = "512"
    notify_complete = False
    terse = False

    for opt in opts:
        if opt[0] == "--inputDir":
            inputDir = opt[1]
        elif opt[0] == "--numSubJobs":
            numSubJobs = int(opt[1])
        elif opt[0] == "--outputTGADir":
            outputTGADir = opt[1]
        elif opt[0] == "--outputAoTGADir":
            outputAoTGADir = opt[1]
        elif opt[0] == "--combinedDDSDir":
            combinedDDSDir = opt[1]
        elif opt[0] == "--atlasedDDSDir":
            atlasedDDSDir = opt[1]
        elif opt[0] == "--compressedDDSDir":
            compressedDDSDir = opt[1]
        elif opt[0] == "--atlasFile":
            atlasFile = opt[1]
        elif opt[0] == "--in_width":
            in_width = opt[1]
        elif opt[0] == "--in_height":
            in_height = opt[1]
        elif opt[0] == "--notify_complete":
            notify_complete == True
        elif opt[0] == "--terse":
            terse = True;

    if not os.path.exists(inputDir):
        print "Invalid input directory"
        return FAIL

    outputTGADir = DefaultDir(outputTGADir, os.path.join(inputDir, "TGA"))
    if not os.path.exists(outputTGADir):
        print "Invalid TGA directory"
        return FAIL

    combinedDDSDir = DefaultDir (combinedDDSDir, os.path.join(inputDir, "PreAtlas"))
    if not os.path.exists(combinedDDSDir):
        print "Invalid combined DDS directory"
        return FAIL

    outputAoTGADir = DefaultDir (outputAoTGADir, os.path.join(inputDir, "AO"))    
    if not os.path.exists(outputAoTGADir):
        print "Invalid AO output TGA directory"
        return FAIL

    compressedDDSDir = DefaultDir (compressedDDSDir, os.path.join(inputDir, "CombinedMaps"))
    if not os.path.exists(compressedDDSDir):
        print "Invalid compressed DDS directory"
        return FAIL

    atlasedDDSDir = DefaultDir (atlasedDDSDir, os.path.join(inputDir, "Atlased"))
    if not os.path.exists(atlasedDDSDir):
        print "Invalid atlased DDS directory"
        return FAIL

    if len(atlasFile) == 0:
        atlasFile = os.path.join(inputDir, "job_atlas.xml")
    if not os.path.exists(atlasFile):
        print "Invalid atlas job file"
        return FAIL

    # Create the input textures
    ProcessFolder(inputDir)

    # Test the raytracing environment for errors
    if aergia.errors.hasErrors():
        print '(%d) errors were detected:' % (aergia.errors.getCount())
        for error in aergia.errors.getErrors():
            print error
        return FAIL

    # Combine the textures
    if not Execute(["BakedMapProcessor.exe", "-in_lm", outputTGADir, 
        "-in_sm", outputTGADir, "-in_ao", outputAoTGADir, "-out", combinedDDSDir]):
        print "FAILED to combine the textures"
        return FAIL

    # Atlas the textures
    if not Execute(["CreateAtlas.exe", "-in", combinedDDSDir, "-out", atlasedDDSDir, 
        "-width", "1024", "-height", "1024", "-gutter", "0", "-halftexel",
        "-in_width", in_width, "-in_height", in_height, "-file", atlasFile]):
        print "FAILED to atlas output textures"
        return FAIL
    for fname in glob.glob(os.path.join(atlasedDDSDir, "*dat.xml")):
        shutil.move(fname, compressedDDSDir)

    # Compress
    if not Execute(["TextureProcessor.exe", "-dxt5", "-nomipmap", 
        "-input_directory", atlasedDDSDir, "-output_directory", compressedDDSDir]):
        print "FAILED to compress atlased textures"
        return FAIL
    for fname in glob.glob(os.path.join(atlasedDDSDir, "*.meta")):
        shutil.copy(fname, compressedDDSDir)

    #Done
    return SUCCESS

if __name__ == '__main__':

    print "Running lightmapper script."
    try:
        retval = main()
        if retval != 0:
            sys.exit(retval)
    except:
        print "Lightmapper script failed."
        print sys.argv
        sys.exit(1)

    print "Lightmapper script completed successfully."



