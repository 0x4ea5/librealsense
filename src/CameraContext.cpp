#include "rs-internal.h"

#include <librealsense/R200/R200.h>
#include <librealsense/F200/F200.h>

rs_context_::rs_context_()
{
	#ifndef WIN32
	uvc_error_t initStatus = uvc_init(&privateContext, NULL);

	if (initStatus < 0)
	{
		uvc_perror(initStatus, "uvc_init");
		throw std::runtime_error("Could not initialize UVC context");
	}
	#endif
	QueryDeviceList();

}

rs_context_::~rs_context_()
{
	cameras.clear(); // tear down cameras before context
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	#ifndef WIN32
	if (privateContext)
	{
		uvc_exit(privateContext);
	}
	#endif
}

void rs_context_::QueryDeviceList()
{
	#ifdef WIN32
	int n = DSGetNumberOfCameras(true);
	for (int i = 0; i < n; ++i)
	{
		uint32_t serialNo = DSGetCameraSerialNumber(i);
		auto ds = DSCreate(DS_DS4_PLATFORM, serialNo);
		if (ds) cameras.push_back(std::make_shared<r200::R200Camera>(ds, i));
	}
	#else
	uvc_device_t **list;

	uvc_error_t status = uvc_get_device_list(privateContext, &list);
	if (status != UVC_SUCCESS)
	{
		uvc_perror(status, "uvc_get_device_list");
		return;
	}

	size_t index = 0;
	while (list[index] != nullptr)
	{
		auto dev = list[index];

		uvc_ref_device(dev);

		uvc_device_descriptor_t * desc;
		if(uvc_get_device_descriptor(dev, &desc) == UVC_SUCCESS)
		{
			if (desc->idProduct == 2688)
			{
				cameras.push_back(std::make_shared<r200::R200Camera>(list[index], index));
			}

			else if (desc->idProduct == 2662)
			{
				cameras.push_back(std::make_shared<f200::F200Camera>(list[index], index));
			}

			uvc_free_device_descriptor(desc);
		}
		index++;
	}

	uvc_free_device_list(list, 1);
	#endif
}

rs_context rs_create_context(rs_error * error) { BEGIN return new rs_context_(); END }
void rs_delete_context(rs_context context, rs_error * error) { BEGIN delete context; END }
int	rs_get_camera_count(rs_context context, rs_error * error) { BEGIN return (int)context->cameras.size(); END }
rs_camera rs_get_camera(rs_context context, int index, rs_error * error) { BEGIN return context->cameras[index].get(); END }