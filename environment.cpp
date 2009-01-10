environment* create_environment()
{
	environment* env = new environment();
	env->out = stdout;
};

void destroy_environment(environment* env)
{
	delete env;
}
