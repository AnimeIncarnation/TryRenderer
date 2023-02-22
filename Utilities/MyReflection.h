//#include <map>
//#include <string>
//#include <functional>
//#include <vector>
//#include <memory>
//
//
//
//class Field
//{
//
//};
//class Method
//{
//
//};
//
//
//class ReflectionSingleton
//{
//public:
//	static ReflectionSingleton* GetReflectionSingleton()
//	{
//		static ReflectionSingleton* rfsgt = new ReflectionSingleton();
//		return rfsgt;
//	}
//
//private:
//	std::map<std::string, std::function<void* (void)>> classMeta;
//	std::map<std::string, std::vector<Field>> fieldMeta;
//	std::map<std::string, std::function<Method>> methodMeta;
//
//public:
//	void RegisterClass() {}
//	void CreateClass() {}
//
//	void RegisterField() {}
//	Field GetClassField() { return Field(); }
//	
//	void RegisterMethod() {}
//	Method GetClassMethod() { return Method(); }
//
//
//};