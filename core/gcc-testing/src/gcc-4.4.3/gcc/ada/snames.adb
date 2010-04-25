------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                               S N A M E S                                --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--          Copyright (C) 1992-2009, Free Software Foundation, Inc.         --
--                                                                          --
-- GNAT is free software;  you can  redistribute it  and/or modify it under --
-- terms of the  GNU General Public License as published  by the Free Soft- --
-- ware  Foundation;  either version 3,  or (at your option) any later ver- --
-- sion.  GNAT is distributed in the hope that it will be useful, but WITH- --
-- OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY --
-- or FITNESS FOR A PARTICULAR PURPOSE.                                     --
--                                                                          --
-- As a special exception under Section 7 of GPL version 3, you are granted --
-- additional permissions described in the GCC Runtime Library Exception,   --
-- version 3.1, as published by the Free Software Foundation.               --
--                                                                          --
-- You should have received a copy of the GNU General Public License and    --
-- a copy of the GCC Runtime Library Exception along with this program;     --
-- see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see    --
-- <http://www.gnu.org/licenses/>.                                          --
--                                                                          --
-- GNAT was originally developed  by the GNAT team at  New York University. --
-- Extensive contributions were provided by Ada Core Technologies Inc.      --
--                                                                          --
------------------------------------------------------------------------------

with Opt;   use Opt;
with Table;
with Types; use Types;

package body Snames is

   --  Table used to record convention identifiers

   type Convention_Id_Entry is record
      Name       : Name_Id;
      Convention : Convention_Id;
   end record;

   package Convention_Identifiers is new Table.Table (
     Table_Component_Type => Convention_Id_Entry,
     Table_Index_Type     => Int,
     Table_Low_Bound      => 1,
     Table_Initial        => 50,
     Table_Increment      => 200,
     Table_Name           => "Name_Convention_Identifiers");

   --  Table of names to be set by Initialize. Each name is terminated by a
   --  single #, and the end of the list is marked by a null entry, i.e. by
   --  two # marks in succession. Note that the table does not include the
   --  entries for a-z, since these are initialized by Namet itself.

   Preset_Names : constant String :=
     "_parent#" &
     "_tag#" &
     "off#" &
     "space#" &
     "time#" &
     "_abort_signal#" &
     "_alignment#" &
     "_assign#" &
     "_atcb#" &
     "_chain#" &
     "_clean#" &
     "_controller#" &
     "_entry_bodies#" &
     "_expunge#" &
     "_final_list#" &
     "_idepth#" &
     "_init#" &
     "_local_final_list#" &
     "_master#" &
     "_object#" &
     "_postconditions#" &
     "_priority#" &
     "_process_atsd#" &
     "_relative_deadline#" &
     "_result#" &
     "_secondary_stack#" &
     "_service#" &
     "_size#" &
     "_stack#" &
     "_tags#" &
     "_task#" &
     "_task_id#" &
     "_task_info#" &
     "_task_name#" &
     "_trace_sp#" &
     "_disp_asynchronous_select#" &
     "_disp_conditional_select#" &
     "_disp_get_prim_op_kind#" &
     "_disp_get_task_id#" &
     "_disp_requeue#" &
     "_disp_timed_select#" &
     "initialize#" &
     "adjust#" &
     "finalize#" &
     "next#" &
     "prev#" &
     "allocate#" &
     "deallocate#" &
     "dereference#" &
     "decimal_io#" &
     "enumeration_io#" &
     "fixed_io#" &
     "float_io#" &
     "integer_io#" &
     "modular_io#" &
     "const#" &
     "<error>#" &
     "go#" &
     "put#" &
     "put_line#" &
     "to#" &
     "exception_traces#" &
     "finalization#" &
     "finalization_root#" &
     "interfaces#" &
     "most_recent_exception#" &
     "standard#" &
     "system#" &
     "text_io#" &
     "wide_text_io#" &
     "wide_wide_text_io#" &
     "no_dsa#" &
     "garlic_dsa#" &
     "polyorb_dsa#" &
     "addr#" &
     "async#" &
     "get_active_partition_id#" &
     "get_rci_package_receiver#" &
     "get_rci_package_ref#" &
     "origin#" &
     "params#" &
     "partition#" &
     "partition_interface#" &
     "ras#" &
     "_call#" &
     "rci_name#" &
     "receiver#" &
     "rpc#" &
     "subp_id#" &
     "operation#" &
     "argument#" &
     "arg_modes#" &
     "handler#" &
     "target#" &
     "req#" &
     "obj_typecode#" &
     "stub#" &
     "Oabs#" &
     "Oand#" &
     "Omod#" &
     "Onot#" &
     "Oor#" &
     "Orem#" &
     "Oxor#" &
     "Oeq#" &
     "One#" &
     "Olt#" &
     "Ole#" &
     "Ogt#" &
     "Oge#" &
     "Oadd#" &
     "Osubtract#" &
     "Oconcat#" &
     "Omultiply#" &
     "Odivide#" &
     "Oexpon#" &
     "ada_83#" &
     "ada_95#" &
     "ada_05#" &
     "ada_2005#" &
     "assertion_policy#" &
     "assume_no_invalid_values#" &
     "c_pass_by_copy#" &
     "check_name#" &
     "check_policy#" &
     "compile_time_error#" &
     "compile_time_warning#" &
     "compiler_unit#" &
     "component_alignment#" &
     "convention_identifier#" &
     "debug_policy#" &
     "detect_blocking#" &
     "discard_names#" &
     "elaboration_checks#" &
     "eliminate#" &
     "extend_system#" &
     "extensions_allowed#" &
     "external_name_casing#" &
     "favor_top_level#" &
     "float_representation#" &
     "implicit_packing#" &
     "initialize_scalars#" &
     "interrupt_state#" &
     "license#" &
     "locking_policy#" &
     "long_float#" &
     "no_run_time#" &
     "no_strict_aliasing#" &
     "normalize_scalars#" &
     "optimize_alignment#" &
     "persistent_bss#" &
     "polling#" &
     "priority_specific_dispatching#" &
     "profile#" &
     "profile_warnings#" &
     "propagate_exceptions#" &
     "queuing_policy#" &
     "ravenscar#" &
     "restricted_run_time#" &
     "restrictions#" &
     "restriction_warnings#" &
     "reviewable#" &
     "source_file_name#" &
     "source_file_name_project#" &
     "style_checks#" &
     "suppress#" &
     "suppress_exception_locations#" &
     "task_dispatching_policy#" &
     "universal_data#" &
     "unsuppress#" &
     "use_vads_size#" &
     "validity_checks#" &
     "warnings#" &
     "wide_character_encoding#" &
     "abort_defer#" &
     "all_calls_remote#" &
     "annotate#" &
     "assert#" &
     "asynchronous#" &
     "atomic#" &
     "atomic_components#" &
     "attach_handler#" &
     "check#" &
     "cil_constructor#" &
     "comment#" &
     "common_object#" &
     "complete_representation#" &
     "complex_representation#" &
     "controlled#" &
     "convention#" &
     "cpp_class#" &
     "cpp_constructor#" &
     "cpp_virtual#" &
     "cpp_vtable#" &
     "debug#" &
     "elaborate#" &
     "elaborate_all#" &
     "elaborate_body#" &
     "export#" &
     "export_exception#" &
     "export_function#" &
     "export_object#" &
     "export_procedure#" &
     "export_value#" &
     "export_valued_procedure#" &
     "external#" &
     "finalize_storage_only#" &
     "ident#" &
     "implemented_by_entry#" &
     "import#" &
     "import_exception#" &
     "import_function#" &
     "import_object#" &
     "import_procedure#" &
     "import_valued_procedure#" &
     "inline#" &
     "inline_always#" &
     "inline_generic#" &
     "inspection_point#" &
     "interface_name#" &
     "interrupt_handler#" &
     "interrupt_priority#" &
     "java_constructor#" &
     "java_interface#" &
     "keep_names#" &
     "link_with#" &
     "linker_alias#" &
     "linker_constructor#" &
     "linker_destructor#" &
     "linker_options#" &
     "linker_section#" &
     "list#" &
     "machine_attribute#" &
     "main#" &
     "main_storage#" &
     "memory_size#" &
     "no_body#" &
     "no_return#" &
     "obsolescent#" &
     "optimize#" &
     "pack#" &
     "page#" &
     "passive#" &
     "postcondition#" &
     "precondition#" &
     "preelaborable_initialization#" &
     "preelaborate#" &
     "preelaborate_05#" &
     "psect_object#" &
     "pure#" &
     "pure_05#" &
     "pure_function#" &
     "relative_deadline#" &
     "remote_call_interface#" &
     "remote_types#" &
     "share_generic#" &
     "shared#" &
     "shared_passive#" &
     "source_reference#" &
     "static_elaboration_desired#" &
     "stream_convert#" &
     "subtitle#" &
     "suppress_all#" &
     "suppress_debug_info#" &
     "suppress_initialization#" &
     "system_name#" &
     "task_info#" &
     "task_name#" &
     "task_storage#" &
     "time_slice#" &
     "title#" &
     "unchecked_union#" &
     "unimplemented_unit#" &
     "universal_aliasing#" &
     "unmodified#" &
     "unreferenced#" &
     "unreferenced_objects#" &
     "unreserve_all_interrupts#" &
     "volatile#" &
     "volatile_components#" &
     "weak_external#" &
     "ada#" &
     "assembler#" &
     "cil#" &
     "cobol#" &
     "cpp#" &
     "fortran#" &
     "intrinsic#" &
     "java#" &
     "stdcall#" &
     "stubbed#" &
     "asm#" &
     "assembly#" &
     "default#" &
     "c_plus_plus#" &
     "dll#" &
     "win32#" &
     "as_is#" &
     "assertion#" &
     "attribute_name#" &
     "body_file_name#" &
     "boolean_entry_barriers#" &
     "casing#" &
     "code#" &
     "component#" &
     "component_size_4#" &
     "copy#" &
     "d_float#" &
     "descriptor#" &
     "dot_replacement#" &
     "dynamic#" &
     "entity#" &
     "entry_count#" &
     "external_name#" &
     "first_optional_parameter#" &
     "form#" &
     "g_float#" &
     "gcc#" &
     "gnat#" &
     "gpl#" &
     "ieee_float#" &
     "ignore#" &
     "info#" &
     "internal#" &
     "link_name#" &
     "lowercase#" &
     "max_entry_queue_depth#" &
     "max_entry_queue_length#" &
     "max_size#" &
     "mechanism#" &
     "message#" &
     "mixedcase#" &
     "modified_gpl#" &
     "name#" &
     "nca#" &
     "no#" &
     "no_dependence#" &
     "no_dynamic_attachment#" &
     "no_dynamic_interrupts#" &
     "no_requeue#" &
     "no_requeue_statements#" &
     "no_task_attributes#" &
     "no_task_attributes_package#" &
     "on#" &
     "parameter_types#" &
     "reference#" &
     "restricted#" &
     "result_mechanism#" &
     "result_type#" &
     "runtime#" &
     "sb#" &
     "secondary_stack_size#" &
     "section#" &
     "semaphore#" &
     "short_descriptor#" &
     "simple_barriers#" &
     "spec_file_name#" &
     "state#" &
     "static#" &
     "stack_size#" &
     "subunit_file_name#" &
     "task_stack_size_default#" &
     "task_type#" &
     "time_slicing_enabled#" &
     "top_guard#" &
     "uba#" &
     "ubs#" &
     "ubsb#" &
     "unit_name#" &
     "unknown#" &
     "unrestricted#" &
     "uppercase#" &
     "user#" &
     "vax_float#" &
     "vms#" &
     "vtable_ptr#" &
     "working_storage#" &
     "abort_signal#" &
     "access#" &
     "address#" &
     "address_size#" &
     "aft#" &
     "alignment#" &
     "asm_input#" &
     "asm_output#" &
     "ast_entry#" &
     "bit#" &
     "bit_order#" &
     "bit_position#" &
     "body_version#" &
     "callable#" &
     "caller#" &
     "code_address#" &
     "component_size#" &
     "compose#" &
     "constrained#" &
     "count#" &
     "default_bit_order#" &
     "definite#" &
     "delta#" &
     "denorm#" &
     "digits#" &
     "elaborated#" &
     "emax#" &
     "enabled#" &
     "enum_rep#" &
     "enum_val#" &
     "epsilon#" &
     "exponent#" &
     "external_tag#" &
     "fast_math#" &
     "first#" &
     "first_bit#" &
     "fixed_value#" &
     "fore#" &
     "has_access_values#" &
     "has_discriminants#" &
     "has_tagged_values#" &
     "identity#" &
     "img#" &
     "integer_value#" &
     "invalid_value#" &
     "large#" &
     "last#" &
     "last_bit#" &
     "leading_part#" &
     "length#" &
     "machine_emax#" &
     "machine_emin#" &
     "machine_mantissa#" &
     "machine_overflows#" &
     "machine_radix#" &
     "machine_rounding#" &
     "machine_rounds#" &
     "machine_size#" &
     "mantissa#" &
     "max_size_in_storage_elements#" &
     "maximum_alignment#" &
     "mechanism_code#" &
     "mod#" &
     "model_emin#" &
     "model_epsilon#" &
     "model_mantissa#" &
     "model_small#" &
     "modulus#" &
     "null_parameter#" &
     "object_size#" &
     "old#" &
     "partition_id#" &
     "passed_by_reference#" &
     "pool_address#" &
     "pos#" &
     "position#" &
     "priority#" &
     "range#" &
     "range_length#" &
     "result#" &
     "round#" &
     "safe_emax#" &
     "safe_first#" &
     "safe_large#" &
     "safe_last#" &
     "safe_small#" &
     "scale#" &
     "scaling#" &
     "signed_zeros#" &
     "size#" &
     "small#" &
     "storage_size#" &
     "storage_unit#" &
     "stream_size#" &
     "tag#" &
     "target_name#" &
     "terminated#" &
     "to_address#" &
     "type_class#" &
     "uet_address#" &
     "unbiased_rounding#" &
     "unchecked_access#" &
     "unconstrained_array#" &
     "universal_literal_string#" &
     "unrestricted_access#" &
     "vads_size#" &
     "val#" &
     "valid#" &
     "value_size#" &
     "version#" &
     "wchar_t_size#" &
     "wide_wide_width#" &
     "wide_width#" &
     "width#" &
     "word_size#" &
     "adjacent#" &
     "ceiling#" &
     "copy_sign#" &
     "floor#" &
     "fraction#" &
     "from_any#" &
     "image#" &
     "input#" &
     "machine#" &
     "max#" &
     "min#" &
     "model#" &
     "pred#" &
     "remainder#" &
     "rounding#" &
     "succ#" &
     "to_any#" &
     "truncation#" &
     "typecode#" &
     "value#" &
     "wide_image#" &
     "wide_wide_image#" &
     "wide_value#" &
     "wide_wide_value#" &
     "output#" &
     "read#" &
     "write#" &
     "elab_body#" &
     "elab_spec#" &
     "storage_pool#" &
     "base#" &
     "class#" &
     "stub_type#" &
     "ceiling_locking#" &
     "inheritance_locking#" &
     "fifo_queuing#" &
     "priority_queuing#" &
     "edf_across_priorities#" &
     "fifo_within_priorities#" &
     "non_preemptive_within_priorities#" &
     "round_robin_within_priorities#" &
     "access_check#" &
     "accessibility_check#" &
     "alignment_check#" &
     "discriminant_check#" &
     "division_check#" &
     "elaboration_check#" &
     "index_check#" &
     "length_check#" &
     "overflow_check#" &
     "range_check#" &
     "storage_check#" &
     "tag_check#" &
     "validity_check#" &
     "all_checks#" &
     "abort#" &
     "abs#" &
     "accept#" &
     "and#" &
     "all#" &
     "array#" &
     "at#" &
     "begin#" &
     "body#" &
     "case#" &
     "constant#" &
     "declare#" &
     "delay#" &
     "do#" &
     "else#" &
     "elsif#" &
     "end#" &
     "entry#" &
     "exception#" &
     "exit#" &
     "for#" &
     "function#" &
     "generic#" &
     "goto#" &
     "if#" &
     "in#" &
     "is#" &
     "limited#" &
     "loop#" &
     "new#" &
     "not#" &
     "null#" &
     "of#" &
     "or#" &
     "others#" &
     "out#" &
     "package#" &
     "pragma#" &
     "private#" &
     "procedure#" &
     "raise#" &
     "record#" &
     "rem#" &
     "renames#" &
     "return#" &
     "reverse#" &
     "select#" &
     "separate#" &
     "subtype#" &
     "task#" &
     "terminate#" &
     "then#" &
     "type#" &
     "use#" &
     "when#" &
     "while#" &
     "with#" &
     "xor#" &
     "divide#" &
     "enclosing_entity#" &
     "exception_information#" &
     "exception_message#" &
     "exception_name#" &
     "file#" &
     "generic_dispatching_constructor#" &
     "import_address#" &
     "import_largest_value#" &
     "import_value#" &
     "is_negative#" &
     "line#" &
     "rotate_left#" &
     "rotate_right#" &
     "shift_left#" &
     "shift_right#" &
     "shift_right_arithmetic#" &
     "source_location#" &
     "unchecked_conversion#" &
     "unchecked_deallocation#" &
     "to_pointer#" &
     "free#" &
     "abstract#" &
     "aliased#" &
     "protected#" &
     "until#" &
     "requeue#" &
     "tagged#" &
     "raise_exception#" &
     "ada_roots#" &
     "aggregate#" &
     "archive_builder#" &
     "archive_builder_append_option#" &
     "archive_indexer#" &
     "archive_suffix#" &
     "binder#" &
     "binder_prefix#" &
     "body_suffix#" &
     "builder#" &
     "builder_switches#" &
     "compiler#" &
     "compiler_kind#" &
     "config_body_file_name#" &
     "config_body_file_name_pattern#" &
     "config_file_switches#" &
     "config_file_unique#" &
     "config_spec_file_name#" &
     "config_spec_file_name_pattern#" &
     "configuration#" &
     "cross_reference#" &
     "default_language#" &
     "default_switches#" &
     "dependency_driver#" &
     "dependency_file_kind#" &
     "dependency_switches#" &
     "driver#" &
     "excluded_source_dirs#" &
     "excluded_source_files#" &
     "excluded_source_list_file#" &
     "exec_dir#" &
     "executable#" &
     "executable_suffix#" &
     "extends#" &
     "externally_built#" &
     "finder#" &
     "global_compilation_switches#" &
     "global_configuration_pragmas#" &
     "global_config_file#" &
     "gnatls#" &
     "gnatstub#" &
     "implementation#" &
     "implementation_exceptions#" &
     "implementation_suffix#" &
     "include_switches#" &
     "include_path#" &
     "include_path_file#" &
     "inherit_source_path#" &
     "language_kind#" &
     "language_processing#" &
     "languages#" &
     "library#" &
     "library_ali_dir#" &
     "library_auto_init#" &
     "library_auto_init_supported#" &
     "library_builder#" &
     "library_dir#" &
     "library_gcc#" &
     "library_interface#" &
     "library_kind#" &
     "library_name#" &
     "library_major_minor_id_supported#" &
     "library_options#" &
     "library_partial_linker#" &
     "library_reference_symbol_file#" &
     "library_src_dir#" &
     "library_support#" &
     "library_symbol_file#" &
     "library_symbol_policy#" &
     "library_version#" &
     "library_version_switches#" &
     "linker#" &
     "linker_executable_option#" &
     "linker_lib_dir_option#" &
     "linker_lib_name_option#" &
     "local_config_file#" &
     "local_configuration_pragmas#" &
     "locally_removed_files#" &
     "map_file_option#" &
     "mapping_file_switches#" &
     "mapping_spec_suffix#" &
     "mapping_body_suffix#" &
     "metrics#" &
     "naming#" &
     "object_generated#" &
     "objects_linked#" &
     "objects_path#" &
     "objects_path_file#" &
     "object_dir#" &
     "path_syntax#" &
     "pic_option#" &
     "pretty_printer#" &
     "prefix#" &
     "project#" &
     "roots#" &
     "required_switches#" &
     "run_path_option#" &
     "runtime_project#" &
     "shared_library_minimum_switches#" &
     "shared_library_prefix#" &
     "shared_library_suffix#" &
     "separate_suffix#" &
     "source_dirs#" &
     "source_files#" &
     "source_list_file#" &
     "spec#" &
     "spec_suffix#" &
     "specification#" &
     "specification_exceptions#" &
     "specification_suffix#" &
     "stack#" &
     "switches#" &
     "symbolic_link_supported#" &
     "sync#" &
     "synchronize#" &
     "toolchain_description#" &
     "toolchain_version#" &
     "runtime_library_dir#" &
     "unaligned_valid#" &
     "interface#" &
     "overriding#" &
     "synchronized#" &
     "#";

   ---------------------
   -- Generated Names --
   ---------------------

   --  This section lists the various cases of generated names which are
   --  built from existing names by adding unique leading and/or trailing
   --  upper case letters. In some cases these names are built recursively,
   --  in particular names built from types may be built from types which
   --  themselves have generated names. In this list, xxx represents an
   --  existing name to which identifying letters are prepended or appended,
   --  and a trailing n represents a serial number in an external name that
   --  has some semantic significance (e.g. the n'th index type of an array).

   --    xxxA    access type for formal xxx in entry param record   (Exp_Ch9)
   --    xxxB    tag table for tagged type xxx                      (Exp_Ch3)
   --    xxxB    task body procedure for task xxx                   (Exp_Ch9)
   --    xxxD    dispatch table for tagged type xxx                 (Exp_Ch3)
   --    xxxD    discriminal for discriminant xxx                   (Sem_Ch3)
   --    xxxDn   n'th discr check function for rec type xxx         (Exp_Ch3)
   --    xxxE    elaboration boolean flag for task xxx              (Exp_Ch9)
   --    xxxE    dispatch table pointer type for tagged type xxx    (Exp_Ch3)
   --    xxxE    parameters for accept body for entry xxx           (Exp_Ch9)
   --    xxxFn   n'th primitive of a tagged type (named xxx)        (Exp_Ch3)
   --    xxxJ    tag table type index for tagged type xxx           (Exp_Ch3)
   --    xxxM    master Id value for access type xxx                (Exp_Ch3)
   --    xxxP    tag table pointer type for tagged type xxx         (Exp_Ch3)
   --    xxxP    parameter record type for entry xxx                (Exp_Ch9)
   --    xxxPA   access to parameter record type for entry xxx      (Exp_Ch9)
   --    xxxPn   pointer type for n'th primitive of tagged type xxx (Exp_Ch3)
   --    xxxR    dispatch table pointer for tagged type xxx         (Exp_Ch3)
   --    xxxT    tag table type for tagged type xxx                 (Exp_Ch3)
   --    xxxT    literal table for enumeration type xxx             (Sem_Ch3)
   --    xxxV    type for task value record for task xxx            (Exp_Ch9)
   --    xxxX    entry index constant                               (Exp_Ch9)
   --    xxxY    dispatch table type for tagged type xxx            (Exp_Ch3)
   --    xxxZ    size variable for task xxx                         (Exp_Ch9)

   --  TSS names

   --    xxxDA   deep adjust routine for type xxx                   (Exp_TSS)
   --    xxxDF   deep finalize routine for type xxx                 (Exp_TSS)
   --    xxxDI   deep initialize routine for type xxx               (Exp_TSS)
   --    xxxEQ   composite equality routine for record type xxx     (Exp_TSS)
   --    xxxFA   PolyORB/DSA From_Any converter for type xxx        (Exp_TSS)
   --    xxxIP   initialization procedure for type xxx              (Exp_TSS)
   --    xxxRA   RAS type access routine for type xxx               (Exp_TSS)
   --    xxxRD   RAS type dereference routine for type xxx          (Exp_TSS)
   --    xxxRP   Rep to Pos conversion for enumeration type xxx     (Exp_TSS)
   --    xxxSA   array/slice assignment for controlled comp. arrays (Exp_TSS)
   --    xxxSI   stream input attribute subprogram for type xxx     (Exp_TSS)
   --    xxxSO   stream output attribute subprogram for type xxx    (Exp_TSS)
   --    xxxSR   stream read attribute subprogram for type xxx      (Exp_TSS)
   --    xxxSW   stream write attribute subprogram for type xxx     (Exp_TSS)
   --    xxxTA   PolyORB/DSA To_Any converter for type xxx          (Exp_TSS)
   --    xxxTC   PolyORB/DSA Typecode for type xxx                  (Exp_TSS)

   --  Implicit type names

   --    TxxxT   type of literal table for enumeration type xxx     (Sem_Ch3)

   --  (Note: this list is not complete or accurate ???)

   ----------------------
   -- Get_Attribute_Id --
   ----------------------

   function Get_Attribute_Id (N : Name_Id) return Attribute_Id is
   begin
      return Attribute_Id'Val (N - First_Attribute_Name);
   end Get_Attribute_Id;

   -----------------------
   -- Get_Convention_Id --
   -----------------------

   function Get_Convention_Id (N : Name_Id) return Convention_Id is
   begin
      case N is
         when Name_Ada        => return Convention_Ada;
         when Name_Assembler  => return Convention_Assembler;
         when Name_C          => return Convention_C;
         when Name_CIL        => return Convention_CIL;
         when Name_COBOL      => return Convention_COBOL;
         when Name_CPP        => return Convention_CPP;
         when Name_Fortran    => return Convention_Fortran;
         when Name_Intrinsic  => return Convention_Intrinsic;
         when Name_Java       => return Convention_Java;
         when Name_Stdcall    => return Convention_Stdcall;
         when Name_Stubbed    => return Convention_Stubbed;

         --  If no direct match, then we must have a convention
         --  identifier pragma that has specified this name.

         when others          =>
            for J in 1 .. Convention_Identifiers.Last loop
               if N = Convention_Identifiers.Table (J).Name then
                  return Convention_Identifiers.Table (J).Convention;
               end if;
            end loop;

            raise Program_Error;
      end case;
   end Get_Convention_Id;

   -------------------------
   -- Get_Convention_Name --
   -------------------------

   function Get_Convention_Name (C : Convention_Id) return Name_Id is
   begin
      case C is
         when Convention_Ada       => return Name_Ada;
         when Convention_Assembler => return Name_Assembler;
         when Convention_C         => return Name_C;
         when Convention_CIL       => return Name_CIL;
         when Convention_COBOL     => return Name_COBOL;
         when Convention_CPP       => return Name_CPP;
         when Convention_Entry     => return Name_Entry;
         when Convention_Fortran   => return Name_Fortran;
         when Convention_Intrinsic => return Name_Intrinsic;
         when Convention_Java      => return Name_Java;
         when Convention_Protected => return Name_Protected;
         when Convention_Stdcall   => return Name_Stdcall;
         when Convention_Stubbed   => return Name_Stubbed;
      end case;
   end Get_Convention_Name;

   ---------------------------
   -- Get_Locking_Policy_Id --
   ---------------------------

   function Get_Locking_Policy_Id (N : Name_Id) return Locking_Policy_Id is
   begin
      return Locking_Policy_Id'Val (N - First_Locking_Policy_Name);
   end Get_Locking_Policy_Id;

   -------------------
   -- Get_Pragma_Id --
   -------------------

   function Get_Pragma_Id (N : Name_Id) return Pragma_Id is
   begin
      if N = Name_AST_Entry then
         return Pragma_AST_Entry;
      elsif N = Name_Fast_Math then
         return Pragma_Fast_Math;
      elsif N = Name_Interface then
         return Pragma_Interface;
      elsif N = Name_Priority then
         return Pragma_Priority;
      elsif N = Name_Relative_Deadline then
         return Pragma_Relative_Deadline;
      elsif N = Name_Storage_Size then
         return Pragma_Storage_Size;
      elsif N = Name_Storage_Unit then
         return Pragma_Storage_Unit;
      elsif N not in First_Pragma_Name .. Last_Pragma_Name then
         return Unknown_Pragma;
      else
         return Pragma_Id'Val (N - First_Pragma_Name);
      end if;
   end Get_Pragma_Id;

   ---------------------------
   -- Get_Queuing_Policy_Id --
   ---------------------------

   function Get_Queuing_Policy_Id (N : Name_Id) return Queuing_Policy_Id is
   begin
      return Queuing_Policy_Id'Val (N - First_Queuing_Policy_Name);
   end Get_Queuing_Policy_Id;

   ------------------------------------
   -- Get_Task_Dispatching_Policy_Id --
   ------------------------------------

   function Get_Task_Dispatching_Policy_Id
     (N : Name_Id) return Task_Dispatching_Policy_Id
   is
   begin
      return Task_Dispatching_Policy_Id'Val
        (N - First_Task_Dispatching_Policy_Name);
   end Get_Task_Dispatching_Policy_Id;

   ----------------
   -- Initialize --
   ----------------

   procedure Initialize is
      P_Index      : Natural;
      Discard_Name : Name_Id;

   begin
      P_Index := Preset_Names'First;
      loop
         Name_Len := 0;
         while Preset_Names (P_Index) /= '#' loop
            Name_Len := Name_Len + 1;
            Name_Buffer (Name_Len) := Preset_Names (P_Index);
            P_Index := P_Index + 1;
         end loop;

         --  We do the Name_Find call to enter the name into the table, but
         --  we don't need to do anything with the result, since we already
         --  initialized all the preset names to have the right value (we
         --  are depending on the order of the names and Preset_Names).

         Discard_Name := Name_Find;
         P_Index := P_Index + 1;
         exit when Preset_Names (P_Index) = '#';
      end loop;

      --  Make sure that number of names in standard table is correct. If
      --  this check fails, run utility program XSNAMES to construct a new
      --  properly matching version of the body.

      pragma Assert (Discard_Name = Last_Predefined_Name);

      --  Initialize the convention identifiers table with the standard
      --  set of synonyms that we recognize for conventions.

      Convention_Identifiers.Init;

      Convention_Identifiers.Append ((Name_Asm,         Convention_Assembler));
      Convention_Identifiers.Append ((Name_Assembly,    Convention_Assembler));

      Convention_Identifiers.Append ((Name_Default,     Convention_C));
      Convention_Identifiers.Append ((Name_External,    Convention_C));

      Convention_Identifiers.Append ((Name_C_Plus_Plus, Convention_CPP));

      Convention_Identifiers.Append ((Name_DLL,         Convention_Stdcall));
      Convention_Identifiers.Append ((Name_Win32,       Convention_Stdcall));
   end Initialize;

   -----------------------
   -- Is_Attribute_Name --
   -----------------------

   function Is_Attribute_Name (N : Name_Id) return Boolean is
   begin
      return N in First_Attribute_Name .. Last_Attribute_Name;
   end Is_Attribute_Name;

   ----------------------------------
   -- Is_Configuration_Pragma_Name --
   ----------------------------------

   function Is_Configuration_Pragma_Name (N : Name_Id) return Boolean is
   begin
      return N in First_Pragma_Name .. Last_Configuration_Pragma_Name
        or else N = Name_Fast_Math;
   end Is_Configuration_Pragma_Name;

   ------------------------
   -- Is_Convention_Name --
   ------------------------

   function Is_Convention_Name (N : Name_Id) return Boolean is
   begin
      --  Check if this is one of the standard conventions

      if N in First_Convention_Name .. Last_Convention_Name
        or else N = Name_C
      then
         return True;

      --  Otherwise check if it is in convention identifier table

      else
         for J in 1 .. Convention_Identifiers.Last loop
            if N = Convention_Identifiers.Table (J).Name then
               return True;
            end if;
         end loop;

         return False;
      end if;
   end Is_Convention_Name;

   ------------------------------
   -- Is_Entity_Attribute_Name --
   ------------------------------

   function Is_Entity_Attribute_Name (N : Name_Id) return Boolean is
   begin
      return N in First_Entity_Attribute_Name .. Last_Entity_Attribute_Name;
   end Is_Entity_Attribute_Name;

   --------------------------------
   -- Is_Function_Attribute_Name --
   --------------------------------

   function Is_Function_Attribute_Name (N : Name_Id) return Boolean is
   begin
      return N in
        First_Renamable_Function_Attribute ..
          Last_Renamable_Function_Attribute;
   end Is_Function_Attribute_Name;

   ---------------------
   -- Is_Keyword_Name --
   ---------------------

   function Is_Keyword_Name (N : Name_Id) return Boolean is
   begin
      return Get_Name_Table_Byte (N) /= 0
        and then (Ada_Version >= Ada_95
                  or else N not in Ada_95_Reserved_Words)
        and then (Ada_Version >= Ada_05
                  or else N not in Ada_2005_Reserved_Words);
   end Is_Keyword_Name;

   ----------------------------
   -- Is_Locking_Policy_Name --
   ----------------------------

   function Is_Locking_Policy_Name (N : Name_Id) return Boolean is
   begin
      return N in First_Locking_Policy_Name .. Last_Locking_Policy_Name;
   end Is_Locking_Policy_Name;

   -----------------------------
   -- Is_Operator_Symbol_Name --
   -----------------------------

   function Is_Operator_Symbol_Name (N : Name_Id) return Boolean is
   begin
      return N in First_Operator_Name .. Last_Operator_Name;
   end Is_Operator_Symbol_Name;

   --------------------
   -- Is_Pragma_Name --
   --------------------

   function Is_Pragma_Name (N : Name_Id) return Boolean is
   begin
      return N in First_Pragma_Name .. Last_Pragma_Name
        or else N = Name_AST_Entry
        or else N = Name_Fast_Math
        or else N = Name_Interface
        or else N = Name_Relative_Deadline
        or else N = Name_Priority
        or else N = Name_Storage_Size
        or else N = Name_Storage_Unit;
   end Is_Pragma_Name;

   ---------------------------------
   -- Is_Procedure_Attribute_Name --
   ---------------------------------

   function Is_Procedure_Attribute_Name (N : Name_Id) return Boolean is
   begin
      return N in First_Procedure_Attribute .. Last_Procedure_Attribute;
   end Is_Procedure_Attribute_Name;

   ----------------------------
   -- Is_Queuing_Policy_Name --
   ----------------------------

   function Is_Queuing_Policy_Name (N : Name_Id) return Boolean is
   begin
      return N in First_Queuing_Policy_Name .. Last_Queuing_Policy_Name;
   end Is_Queuing_Policy_Name;

   -------------------------------------
   -- Is_Task_Dispatching_Policy_Name --
   -------------------------------------

   function Is_Task_Dispatching_Policy_Name (N : Name_Id) return Boolean is
   begin
      return N in First_Task_Dispatching_Policy_Name ..
                  Last_Task_Dispatching_Policy_Name;
   end Is_Task_Dispatching_Policy_Name;

   ----------------------------
   -- Is_Type_Attribute_Name --
   ----------------------------

   function Is_Type_Attribute_Name (N : Name_Id) return Boolean is
   begin
      return N in First_Type_Attribute_Name .. Last_Type_Attribute_Name;
   end Is_Type_Attribute_Name;

   ----------------------------------
   -- Record_Convention_Identifier --
   ----------------------------------

   procedure Record_Convention_Identifier
     (Id         : Name_Id;
      Convention : Convention_Id)
   is
   begin
      Convention_Identifiers.Append ((Id, Convention));
   end Record_Convention_Identifier;

end Snames;
