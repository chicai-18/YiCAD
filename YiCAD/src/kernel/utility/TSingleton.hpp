#ifndef TSINGLETON_H
#define TSINGLETON_H

/// @brief 单例模式
template <class T>
class TSingleton
{
public:
	virtual ~TSingleton() {}

	static T* GetInstance()
	{
		if (nullptr == s_pInsatcne)
		{
			s_pInsatcne = new T();
		}
		
		return s_pInsatcne;
	}

	static void ReleaseInstance()
	{
		if (nullptr != s_pInsatcne)
		{
			delete s_pInsatcne;
			s_pInsatcne = nullptr;
		}
	}
protected:
	TSingleton() {}
	TSingleton(const TSingleton&) = delete;
	TSingleton(const TSingleton&&) = delete;
	TSingleton& operator = (TSingleton&) = delete;
	TSingleton& operator = (TSingleton&&) = delete;
private:
	static T*			s_pInsatcne;
};

template <class T>
T* TSingleton<T>::s_pInsatcne = nullptr;

#endif
