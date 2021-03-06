/**
* @file cmn_pool_tag.h
* @version 0.1.0.1
* @copyright (2018) TwoSix Labs
* @brief define module scoped pool tags to assist with
* memory leak debugging
*/

#pragma once

#define NEW_POOL_TAG						' weN'
#define CPPRUNTIME_POOL_TAG					'TRPC'
#define FILTERMANAGER_POOL_TAG				'rtlF'
#define INSTANCECONTEXT_POOL_TAG			'xtci'
#define STREAMCONTEXT_POOL_TAG				'xtcs'
#define WVU_FLT_REG_POOL_TAG				'grlf'
#define WVU_QUERY_VOLUME_NAME_POOL_TAG		'lovq'
#define WVU_DEBUG_POOL_TAG					'gbed'
#define WVU_CONTEXT_POOL_TAG				' xtc'
#define FLTMGR_OP_CALLBACK_POOL_TAG			'bcpo'
#define WVU_REGISTRY_MODIFICATION_POOL_TAG	'domr'
#define WVU_THREAD_POOL_TAG					'vwbo'
#define WVU_DRIVER_POOL_TAG					'rvrd'
#define WVU_PROBEDATAQUEUE_POOL_TAG         'qdrp'
#define WVU_IMAGELOADPROBE_POOL_TAG         'bpli'
#define WVU_PROCESSCTORDTORPROBE_POOL_TAG   'corp'
#define WVU_PROCLISTVALIDPROBE_POOL_TAG     'lavp'
#define WVU_ABSTRACTPROBE_POOL_TAG	        'ebrp'
#define WVU_FLTCOMMSMGR_POOL_TAG            'mocf'
#define WVU_FLTCOMMSQUEUE_POOL_TAG          'qmoc'
#define WVU_PROBEINFO_POOL_TAG              'borp'
#define WVU_WVUMGR_POOL_TAG					'muvw'
#define WVU_PROCESSENTRY_POOL_TAG			'tnep'
#define WVU_THREADCREATEPROBE_POOL_TAG      'bpct'