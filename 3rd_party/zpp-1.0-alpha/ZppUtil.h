#ifdef ZPP_INCLUDE_OPENALL
// have to have this function
typedef std::list<std::string> stringList;
extern stringList *enumerateDir(const std::string &wildPath, bool fullPath);
#endif /* ZPP_INCLUDE_OPENALL */
