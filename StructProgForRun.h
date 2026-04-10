

struct TableRedirect{
	int* fd_streams;
	char **filename;
	int add_in_file;
};


struct Cmd{
	char *word;
	struct Cmd *next;
};


struct ProgForRun{
	struct Cmd **command;
	struct TableRedirect **table;
	enum ConstForTypeCmd *type_cmd;

	int count;
	int size;

};

