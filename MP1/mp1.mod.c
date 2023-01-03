#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x4dc71e1, "module_layout" },
	{ 0x2b68bd2f, "del_timer" },
	{ 0x8c03d20c, "destroy_workqueue" },
	{ 0x42160169, "flush_workqueue" },
	{ 0xce554427, "proc_remove" },
	{ 0x49cd25ed, "alloc_workqueue" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x31296ee7, "proc_create" },
	{ 0x7aa69493, "proc_mkdir" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x2d5f69b3, "rcu_read_unlock_strict" },
	{ 0x4098a4d2, "pid_task" },
	{ 0x60064642, "find_vpid" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x54b1fac6, "__ubsan_handle_load_invalid_value" },
	{ 0x8da6585d, "__stack_chk_fail" },
	{ 0x37a0cba, "kfree" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0xcefb0c9f, "__mutex_init" },
	{ 0xbcab6ee6, "sscanf" },
	{ 0x6031e814, "kmem_cache_alloc_trace" },
	{ 0x12c95864, "kmalloc_caches" },
	{ 0xdcb764ad, "memset" },
	{ 0x12a4e128, "__arch_copy_from_user" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0x56470118, "__warn_printk" },
	{ 0xb788fb30, "gic_pmr_sync" },
	{ 0x4b0a3f52, "gic_nonsecure_priorities" },
	{ 0x5e2d7875, "cpu_hwcap_keys" },
	{ 0x5c1c3eb4, "cpu_hwcaps" },
	{ 0x14b89635, "arm64_const_caps_ready" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0x92997ed8, "_printk" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "44FC25499EF191F5D672430");
