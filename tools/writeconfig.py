import os
import sys

package='YafaRay'
version='0.1.0'
config_file='yafray_config.h'


def write_conf(env):
	double_coords=0
	yafray_namespace='yafray'
	if double_coords :
		min_raydist="0.000000000005"
	else:
		min_raydist="0.00005"
	
	if os.path.exists(config_file):
		print "Using config file: "+config_file
	else:
		print "Creating config file:"+config_file
		config=open(config_file,'w')
		config.write("//Config file header created by scons\n\n")
		config.write("#ifndef Y_CONFIG_H\n")
		config.write("#define Y_CONFIG_H\n")
		config.write("#include \"yafray_constants.h\"\n\n")
		config.write("#define MIN_RAYDIST %s\n"%(min_raydist))
		config.write("\n")
		config.write("__BEGIN_YAFRAY\n");
		config.write("typedef float CFLOAT;\n");
		config.write("typedef float GFLOAT;\n");
		if double_coords:
			config.write("typedef double PFLOAT;\n");
		else:
			config.write("typedef float PFLOAT;\n");
		config.write("__END_YAFRAY\n");

		#if sys.platform == 'win32' :
		#	config.write("#ifdef BUILDING_YAFRAYCORE\n")
		#	config.write("#define YAFRAYCORE_EXPORT __declspec(dllexport)\n")
		#	config.write("#else \n")
		#	config.write("#define YAFRAYCORE_EXPORT __declspec(dllimport)\n")
		#	config.write("#endif \n")
		#
		#	config.write("#ifdef BUILDING_YAFRAYPLUGIN\n")
		#	config.write("#define YAFRAYPLUGIN_EXPORT __declspec(dllexport)\n")
		#	config.write("#else \n")
		#	config.write("#define YAFRAYPLUGIN_EXPORT __declspec(dllimport)\n")
		#	config.write("#endif \n")
		#else :
		#	config.write("#define YAFRAYPLUGIN_EXPORT\n")
		#	config.write("#define YAFRAYCORE_EXPORT\n")

		config.write("#endif\n");
		config.close()

def write_rev(env):
	rev = os.popen('svnversion').readline().strip()
	rev_file=open('yaf_revision.h','w')
	if rev == 'exported' or rev == '': rev = "N/A"
	
	rev_file.write("#ifndef Y_REV_H\n")
	rev_file.write("#define Y_REV_H\n")
	rev_file.write("#define YAF_SVN_REV \""+rev+"\"\n")
	
	rev_file.write("#define LIBPATH \"%s\"\n"%( env.subst('$YF_LIBOUT') ) )
	rev_file.write("#define Y_PLUGINPATH \"%s\"\n#endif\n"%( env.subst('$YF_PLUGINPATH') ) )

