# @file middleware_handler.py
#
# @brief Generate YAML description and fragments
#
# @author Thomas Vanderbruggen <thomas@koheron.com>
# @date 02/09/2015
#
# (c) Koheron 2014-2015

import os
import time
import yaml

class MiddlewareHandler:
    """ Generate YAML description and fragments    
    """
    
    def __init__(self, hppfile):
       self.parser =  MiddlewareHppParser(hppfile)
       
    def generate_device_description_file(self, path):
        """Generate the YAML device description file
        """
        d2l_filename = os.path.join(path, self.parser.get_device_name().lower() + ".yaml")
        f = open(d2l_filename, 'w')
        
        f.write('# @file ' + os.path.basename(d2l_filename) + '\n')
        f.write('#\n')
        f.write('# @brief Device description file for ' + self.parser.raw_dev_data["name"] + '\n')
        f.write('#\n')
        f.write('# Autogenerated by Koheron Devgen\n')
        f.write('# ' + time.strftime("%c") + "\n")
        f.write('# (c) Koheron 2014-2015 \n\n')

        yaml.dump(self.parser.device, f, indent=2, default_flow_style=False)
        f.close()
        
        return d2l_filename
        
    def get_device_data(self):
        # self.generate_device_description_file('./tmp')
        return self.parser.device
        
    def get_fragments(self):
        frag_gen = FragmentsGenerator(self.parser)
        return frag_gen.get_fragments()
        
        
def ERROR(line, message):
    raise ValueError("Error line " + str(line) + ": " + message)
    
def WARNING(line, message):
    print "Warning line " + str(line) + ": " + message

class MiddlewareHppParser:
    def __init__(self, hppfile):
        fd = open(hppfile, 'r')
        
        line_cnt = 0;
        
        device = {}
        device["operations"] = [];
        device["includes"] = [self._get_include(hppfile)];
        operation = {}
        operation["params"] = []
        operation["flags"] = []
        op_line = ""
        
        is_dev_general = True  # True if we are parsing the general properties of the device
        is_operation = False   # True if we are parsing an operation
        is_failed = False      # True if parsing is_failed function
        
        for line in fd.readlines():
            line_cnt = line_cnt + 1
            line_strip = line.strip()
            
            # Eliminate comments
            if line_strip[:2] == "//" and line_strip[:3] != "//>":
                continue
            
            tokens = line_strip.split(' ')
            
            if tokens[0] == '//>' and len(tokens) > 1:
                    
                tag = tokens[1].strip()
                
                # Determine whether a new operation is being defined
                if not is_operation and not is_dev_general and not tag == '\pool':
                    is_operation = True
            
                if tag == '\description':
                    if len(tokens) > 2:
                        if is_dev_general:
                            device["description"] = ' '.join(tokens[2:])
                        elif is_operation:
                            operation["description"] = ' '.join(tokens[2:])
                        else:
                            WARNING(line_cnt, "Tag \description doesn't refer to any device or operation")
                    else:
                        if is_dev_general:
                            WARNING(line_cnt, "Empty device description")
                        elif is_operation:
                            WARNING(line_cnt, "Empty operation description")
                elif tag == '\io_type':
                    if is_operation:
                        io_type = tokens[2].strip()
                        io_type_remaining = ""
                        
                        if io_type == "READ_ARRAY": # \io_type READ_ARRAY length
                            if len(tokens) <= 3:
                                ERROR(line_cnt, "No array length specified for I/O type READ_ARRAY")
                            
                            io_type_remaining = ' '.join(tokens[3:])
                        elif io_type == "WRITE_ARRAY": # \io_type WRITE_ARRAY ptr_name length
                            if len(tokens) <= 4:
                                ERROR(line_cnt, "Array pointer name and length must " 
                                                + "be specified for I/O type WRITE_ARRAY")
                             
                            io_type_remaining = ' '.join(tokens[3:])
                        operation["io_type"] = {
                          "value": io_type,
                          "remaining": io_type_remaining
                        }
                    else:
                        ERROR(line_cnt, "Tag \io_type defined outside an operation")
                elif tag == '\param':
                    if len(tokens) > 3:
                        operation["params"].append({
                          "name": tokens[2],
                          "description": ' '.join(tokens[3:])
                        });
                    else:
                        WARNING(line_cnt, "Empty parameter name and/or description")
                elif tag == '\status':
                    if is_operation:
                        operation["status"] = tokens[2].strip()
                    else:
                        ERROR(line_cnt, "Tag \status defined outside an operation")
                elif tag == '\on_error':
                    if is_operation:
                        operation["on_error"] = ' '.join(tokens[2:])
                    else:
                        ERROR(line_cnt, "Tag \on_error defined outside an operation")
                elif tag == '\\flag':
                    if is_operation:
                        for i in range(2, len(tokens)):
                            operation["flags"].append({"name": tokens[i].strip()})
                    else:
                        ERROR(line_cnt, "Tag \\flag defined outside an operation")
                elif tag == '\is_failed':
                    is_operation = False
                    is_failed = True
            elif tokens[0] == 'class':
                if is_dev_general == True:
                    if len(tokens) > 1:
                        classname = tokens[1].strip()
                        
                        device["objects"] = []
                        device["objects"].append({
                          "name": classname.lower(),
                          "type": classname
                        })
                        
                        device["name"] = classname
                    else:
                        ERROR(line_cnt, "No device class name")
                    
                    is_dev_general = False
            elif is_operation:
            
                # We accumulate the lines until either ';' or '{' is find,
                # indicating the function prototype is complete.
                
                op_line += line
                reset_op = False
                
                try:
                    if op_line.find(';') != -1:
                        operation["prototype"] = self._parse_function_prototype(op_line.split(';')[0])
                        reset_op = True
                    elif op_line.find('{') != -1:
                        operation["prototype"] = self._parse_function_prototype(op_line.split('{')[0])
                        reset_op = True
                except:
                    ERROR(line_cnt, "Invalid function prototype")
                
                if reset_op:
                    if (not "status" in operation) or (operation["status"] == ""):
                        # By default NEVER_FAIL
                        operation["status"] = "NEVER_FAIL"
                        
                    if operation["io_type"]["value"] == "WRITE_ARRAY":
                        operation["array_params"] = self._get_write_array_params(operation["io_type"])

                    self._check_operation(operation)
                    device["operations"].append(operation)
                    operation = {}
                    operation["params"] = []
                    operation["flags"] = []
                    op_line = ""
                    is_operation = False
            elif is_failed:
                device["is_failed"] = self._parse_function_prototype(line)
                
                if device["is_failed"]["ret_type"] != "bool":
                    ERROR(line_cnt, "Failure indicator function must return a bool")
                    
                if len(device["is_failed"]["params"]) > 0:
                    ERROR(line_cnt, "Failure indicator function mustn't have argument")
                 
                is_failed = False
                
        if not "description" in device:
            device["description"] = ""
                
        self.raw_dev_data = device
        self.device = self._get_device()
        fd.close()
        
    def _get_include(self, hppfile):
        folders = hppfile.split('/')        
        middleware_idx = -1
        
        for idx, folder in enumerate(folders):
            if folder == "middleware":
                middleware_idx = idx
                break
        
        if middleware_idx == -1:
            raise ValueError("Source file must be in the middleware folder")

        return '/'.join(folders[(middleware_idx+1):])
        
    def _parse_function_prototype(self, line):
        function = {}
        function["params"] = []
        
        tokens = line.split('(')
        
        # Parse return type and name
        tmp_toks = tokens[0].strip().split(' ')
        function["ret_type"] = ' '.join(tmp_toks[0:len(tmp_toks)-1])
        # TODO Check return type is a valid KServer type        
        function["name"] = tmp_toks[len(tmp_toks)-1].strip()
        
        # Parse the parameters
        params_toks = tokens[1].split(')')[0].split(',')
        
        if params_toks[0] != '':
            for param_tok in params_toks:
                param = {}
                dflt_value_toks = param_tok.strip().split('=')
                
                if dflt_value_toks == 2:
                    param["default"] = dflt_value_toks[1].strip()
                    
                type_name_toks__ = dflt_value_toks[0].split(' ')
                type_name_toks = []
                
                for tok in type_name_toks__:
                    if tok != "":
                        type_name_toks.append(tok)
                
                param["type"] = ' '.join(type_name_toks[0:len(type_name_toks)-1])
                # TODO Check param type is a valid KServer type
                param["name"] = type_name_toks[len(type_name_toks)-1].strip()
                function["params"].append(param)
        
        return function
        
    def _get_template(self, ret_type):
        tokens = ret_type.split('<')
        
        if len(tokens) == 2:
            return tokens[1].split('>')[0].strip()
        else:
            return None
        
    def _check_operation(self, operation):
        # Check execution status definition
        if not self._is_valid_status(operation["status"]):
            raise ValueError("In operation " 
                             + operation["prototype"]["name"].upper() 
                             + ": Invalid execution status " 
                             + operation["status"])
        elif(operation["status"] == "ERROR_IF_NEG" 
             and operation["prototype"]["ret_type"] != "int"):
            raise ValueError("In operation " 
                             + operation["prototype"]["name"].upper() 
                             + ": Invalid return type for status ERROR_IF_NEG\n" 
                             + "Expected int")
                             
        if not "io_type" in operation:
            raise ValueError("In operation " 
                             + operation["prototype"]["name"].upper() 
                             + ": No I/O type specified")
        elif not self._is_valid_io_type(operation["io_type"]["value"]):
            raise ValueError("In operation " 
                             + operation["prototype"]["name"].upper() 
                             + ": Invalid I/O type " 
                             + operation["io_type"]["value"])
                             
        # Check flags
        if len(operation["flags"]) > 0:
            for flag in operation["flags"]: 
                if not self._is_valid_flag(flag["name"]):
                    raise ValueError("In operation " 
                             + operation["prototype"]["name"].upper() 
                             + ": Invalid flag " + flag["name"])
                             
        # Check parameter names
        if len(operation["params"]) > 0:
            for param in operation["params"]:
                if not self._is_defined_param(operation, param["name"]):
                    raise ValueError("In operation " 
                                 + operation["prototype"]["name"].upper() 
                                 + ": unknown parameter name " + param["name"])
                             
    def _is_valid_status(self, status):
        return (
          status == "ERROR_IF_NEG"  or
          status == "ERROR_IF_NULL" or
          status == "NEVER_FAIL"
        )
        
    def _is_valid_io_type(self, io_type):
        return (
          io_type == "WRITE"      or
          io_type == "READ"       or
          io_type == "READ_CSTR"  or
          io_type == "READ_ARRAY" or
          io_type == "WRITE_ARRAY"
        )
        
    def _is_valid_flag(self, flag):
        return (
          flag == "AT_INIT"
        )
        
    def _is_defined_param(self, operation, param_name):
        for func_param in operation["prototype"]["params"]:
            if func_param["name"] == param_name:
                return True
                
        return False
        
    def save_raw_data(self, yaml_filename):
        """Save raw device data in YAML
        
        For developement purpose mainly
        """
        f = open(yaml_filename, 'w')
        
        f.write('# @file ' + os.path.basename(yaml_filename) + '\n')
        f.write('#\n')
        f.write('# @brief Raw parsing of device ' + self.raw_dev_data["name"] + '\n')
        f.write('#\n')
        f.write('# Autogenerated by Koheron Devgen\n')
        f.write('# ' + time.strftime("%c") + "\n")
        f.write('# (c) Koheron 2014-2015 \n\n')
    
        yaml.dump(self.raw_dev_data, f, indent=2, default_flow_style=False)
        f.close()
        
    def _get_device(self):
        device = {}
        device["operations"] = []
        device["name"] = self.get_device_name()
        
        if("description" in self.raw_dev_data 
           and self.raw_dev_data["description"] != ""):
            device["description"] = self.raw_dev_data["description"]
        else:
            device["description"] = ""
            
        device["includes"] = self.raw_dev_data["includes"]
        device["objects"] = [{
          "type": self.raw_dev_data["objects"][0]["type"],
          "name": "__" + self.get_device_name().lower()
        }]
        
        for op in self.raw_dev_data["operations"]:
            device["operations"].append(self._format_operation(op))
            
        return device
        
    def get_device_name(self):
        """ Build the device name from the class name
        """
        raw_name = self.raw_dev_data["name"]
        
        dev_name = []
        
        # Check whether there are capital letters within the class name
        # and insert an underscore before them
        for idx, letter in enumerate(raw_name):
            if idx > 0 and letter == letter.upper():
                dev_name.append('_')
                
            dev_name.append(letter.upper())
            
        return ''.join(dev_name)
        
    def _format_operation(self, op):
        operation = {}
        operation["name"] = op["prototype"]["name"].upper()
            
        if "description" in op and op["description"] != "":
            operation["description"] = op["description"]
            
        if len(op["flags"]) > 0:
            operation["flags"] = op["flags"]
            
        if op["io_type"]["value"] == "WRITE_ARRAY":            
            send_buffer_flag = {
              "name": "SEND_BUFFER",
              "buffer_name": [op["array_params"]["name"]["name"]]
            }
            
            if len(op["flags"]) > 0:
                operation["flags"].append(send_buffer_flag)
            else:
                operation["flags"] = [send_buffer_flag]
            
        if len(op["prototype"]["params"]) > 0:
            if op["io_type"]["value"] == "WRITE_ARRAY":                
                array_name_ok = False
                array_lenght_ok = False
                
                for param in op["prototype"]["params"]:
                    param_ptr_toks = param["name"].split('*')
                
                    if (op["array_params"]["name"]["src"] == "param" and 
                        len(param_ptr_toks) == 2 and
                        param_ptr_toks[1].strip() == op["array_params"]["name"]["name"]):
                        array_name_ok = True
                    elif (op["array_params"]["name"]["src"] == "param" and 
                        len(param_ptr_toks) == 1 and
                        param_ptr_toks[0].strip() == op["array_params"]["name"]["name"]):
                        array_name_ok = True
                    elif (op["array_params"]["length"]["src"] == "param" and 
                          param["name"] == op["array_params"]["length"]["length"]):
                        array_lenght_ok = True
                    else: # Miscellaneous parameter
                        arg = {}
                        arg["name"] = param["name"]
                        arg["type"] = param["type"]
                        arg["description"] = self._get_param_desc(op, param["name"])
                        
                        if "arguments" in operation:
                            operation["arguments"].append(arg)
                        else:
                            operation["arguments"] = [arg]
            else:
                operation["arguments"] = []
                
                for param in op["prototype"]["params"]:
                    arg = {}
                    arg["name"] = param["name"]
                    arg["type"] = param["type"]
                    arg["description"] = self._get_param_desc(op, param["name"])
                    
                    operation["arguments"].append(arg)
            
        return operation
        
    def _get_param_desc(self, operation, param_name):
        for param in operation["params"]:
            if param["name"] == param_name:
                return param["description"]
                
        return "" # No description provided
        
    def _get_array_length(self, io_type_remains):
        tokens = io_type_remains.split('=>')
        
        if len(tokens) != 2:
            raise ValueError("Invalid array length specification. Expect src=>length")
            
        return {
          "src": tokens[0].strip(),
          "length": tokens[1].strip()
        }
        
    def _get_array_name(self, io_type_remains):
        tokens = io_type_remains.split('=>')
        
        if len(tokens) != 2:
            raise ValueError("Invalid array name specification. Expect src=>name")
            
        return {
          "src": tokens[0].strip(),
          "name": tokens[1].strip()
        }
        
    def _get_write_array_params(self, io_type):
        assert io_type["value"] == "WRITE_ARRAY"
        
        tokens = io_type["remaining"].split(' ')
        
        if len(tokens) != 2:
            raise ValueError("WRITE_ARRAY expects pointer name and length")
        
        return {
            "name": self._get_array_name(tokens[0]),
            "length": self._get_array_length(tokens[1])
        }
        
class FragmentsGenerator:
    def __init__(self, parser):
        self.parser = parser
        
    def get_fragments(self):
        fragments = []
        
        for op in self.parser.raw_dev_data["operations"]:
            op_name = op["prototype"]["name"].upper()
            frag = {}
            frag["name"] = op_name
            frag["fragment"] = self.generate_fragment(op_name)
            fragments.append(frag)
        
        # Add is_failed fragment 
        frag = {}
        frag["name"] = "IS_FAILED"
        frag["fragment"] = [self._gen_is_failed_fragment()]
        fragments.append(frag)
        
        return fragments
        
    def generate_fragment(self, op_name):
        """ Generate the fragment of an operation
        """
        operation = self._get_operation_data(op_name)
        
        frag = []
        
        if operation["io_type"]["value"] == "WRITE":
            if "on_error" in operation:
                if operation["status"] == "ERROR_IF_NEG":
                    frag.append("    if (" + self._build_func_call(operation) + " < 0) {\n")
                    frag.append('        kserver->syslog.print(SysLog::ERROR, "' 
                                         + operation["on_error"] + '\\n");\n')
                    frag.append("        return -1;\n")
                    frag.append("    }\n\n")
                    
                    frag.append("    return 0;\n")
            else:
                if operation["status"] == "ERROR_IF_NEG":
                     frag.append("    return " + self._build_func_call(operation) + ";\n")
                elif operation["status"] == "NEVER_FAIL":
                    frag.append("    " + self._build_func_call(operation) + ";\n")
                    frag.append("    return 0;\n")
        elif operation["io_type"]["value"] == "READ":
            if operation["status"] == "ERROR_IF_NEG":
                # In this case the return type must be an int.
                # After checking the status, we send the 
                # result as an uint32_t to the client
                
                frag.append("    int ret = " + self._build_func_call(operation) + ";\n\n")
                    
                frag.append("    if (ret < 0) {\n")
                if "on_error" in operation:
                    frag.append('        kserver->syslog.print(SysLog::ERROR, "' 
                                + operation["on_error"] + '\\n");\n')
                frag.append("        return -1;\n")
                frag.append("    }\n\n")
                    
                frag.append("    return SEND<uint32_t>(ret);\n")
            elif operation["status"] == "NEVER_FAIL":
                template = self.parser._get_template(operation["prototype"]["ret_type"])
            
                if operation["prototype"]["ret_type"] == "uint32_t":
                    frag.append("    return SEND<uint32_t>(" 
                                + self._build_func_call(operation) + ");\n")
                elif template != None:
                    type_base = operation["prototype"]["ret_type"].split('<')[0].strip()
                    
                    if (type_base == "Klib::KVector" 
                        or type_base == "std::vector" 
                        or type_base == "std::tuple"):
                        frag.append("    return SEND<" + template + ">(" 
                                    + self._build_func_call(operation) + ");\n")
                else:
                    raise ValueError("No available interface to send type " 
                                     + operation["prototype"]["ret_type"])
        
        elif operation["io_type"]["value"] == "READ_CSTR":
            if (operation["prototype"]["ret_type"] != "char *"       and
                operation["prototype"]["ret_type"] != "char*"        and
                operation["prototype"]["ret_type"] != "const char *" and
                operation["prototype"]["ret_type"] != "const char*"):
                raise ValueError("I/O type READ_CSTR expects a char*. Found " 
                                 + operation["prototype"]["ret_type"] + ".\n")
                      
            if operation["status"] == "ERROR_IF_NULL":
                frag.append("    char *ret = " + self._build_func_call(operation) + ";\n\n")
                
                frag.append("    if (ret == nullptr) {\n")
                if "on_error" in operation:
                    frag.append('        kserver->syslog.print(SysLog::ERROR, "' 
                                + operation["on_error"] + '\\n");\n')
                frag.append("        return -1;\n")
                frag.append("    }\n\n")
                
                frag.append("    return SEND_CSTR(ret);\n")
            elif operation["status"] == "NEVER_FAIL":
                    frag.append("    return SEND_CSTR(" 
                                + self._build_func_call(operation) + ");\n")
            
        elif operation["io_type"]["value"] == "READ_ARRAY":
            ptr_type = self._get_ptr_type(operation["prototype"]["ret_type"])
            
            #Get array length
            len_data = self._get_array_length(operation["io_type"]["remaining"])
            
            if len_data["src"] == "this":
                obj_name = self.device["objects"][0]["name"]
                length = "THIS->" + obj_name + "." + len_data["length"]
            elif len_data["src"] == "param":
                # TODO Check this is a valid param
                length = "args." + len_data["length"]
            else:
                raise ValueError("Unknown array length source")
            
            if operation["status"] == "ERROR_IF_NULL":
                frag.append("    " + ptr_type + " *ret = " + self._build_func_call(operation) + ";\n\n")
                
                frag.append("    if (ret == nullptr) {\n")
                if "on_error" in operation:
                    frag.append('        kserver->syslog.print(SysLog::ERROR, "' 
                                + operation["on_error"] + '\\n");\n')
                frag.append("        return -1;\n")
                frag.append("    }\n\n")
                
                frag.append("    return SEND_ARRAY<" + ptr_type + ">(ret, " + length + ");\n")
            elif operation["status"] == "NEVER_FAIL":
                frag.append("    return SEND_ARRAY<" + ptr_type + ">(" 
                            + self._build_func_call(operation) + ", " + length + ");\n")
                            
        elif operation["io_type"]["value"] == "WRITE_ARRAY":
            len_name = "len_" + operation["array_params"]["name"]["name"]
            
            if operation["status"] == "NEVER_FAIL":
                frag.append("    const uint32_t *data_ptr = RCV_HANDSHAKE(args." + len_name + ");\n\n")
                
                frag.append("    if(data_ptr == nullptr) {\n")
                frag.append("       return -1;\n")
                frag.append("    }\n\n")
                
                frag.append("    " + self._build_write_array_func_call(operation) + ";\n\n")
                frag.append("    return 0;\n")
                            
        # self._show_fragment(frag)
        return frag
        
    def _get_ptr_type(self, ret_type):
        """Get the pointer type
        
        Ex. if ret_type is char* it returns char
        
        Raise an error if ret_type is not a pointer
        """
        tokens = ret_type.split('*')
        
        # T*
        if len(tokens) == 2:
            return tokens[0].strip()
        # const T*
        elif tokens[0].split(' ')[0].strip() == "const" and len(tokens) == 2:
            return tokens[0].split(' ')[1].strip()
        else:
            raise ValueError("Return type " + ret_type + " is not a pointer")
        
    def _get_operation_data(self, op_name):
        for op in self.parser.raw_dev_data["operations"]:
            if op["prototype"]["name"].upper() == op_name:
                return op
                
        raise ValueError("Unknown operation " + op_name)
        
    def _build_func_call(self, operation):
        obj_name = self.parser.device["objects"][0]["name"]
        func_name = operation["prototype"]["name"]
        
        call = "THIS->" + obj_name + "." + func_name + "("
        
        for count, param in enumerate(operation["prototype"]["params"]):
            if count == 0:
                call += "args." + param["name"]
            else:
                call += ", args." + param["name"]
        
        call += ")" 
        
        return call
        
    def _build_write_array_func_call(self, operation):
        assert operation["io_type"]["value"] == "WRITE_ARRAY"
        
        len_name = "len_" + operation["array_params"]["name"]["name"]
    
        obj_name = self.parser.device["objects"][0]["name"]
        func_name = operation["prototype"]["name"]
        
        call = "THIS->" + obj_name + "." + func_name + "("
                
        for count, param in enumerate(operation["prototype"]["params"]):
            param_ptr_toks = param["name"].split('*')
            
            if ((operation["array_params"]["name"]["src"] == "param" and 
                len(param_ptr_toks) == 2 and 
                param_ptr_toks[1].strip() == operation["array_params"]["name"]["name"]) or
                (operation["array_params"]["name"]["src"] == "param" and 
                len(param_ptr_toks) == 1 and 
                param_ptr_toks[0].strip() == operation["array_params"]["name"]["name"])):
                
                if count == 0:
                    call += "data_ptr"
                else:
                    call += ", data_ptr"
            elif (operation["array_params"]["length"]["src"] == "param" and 
                  param["name"] == operation["array_params"]["length"]["length"]):
                if count == 0:
                    call += "args." + len_name
                else:
                    call += ", args." + len_name
            else:
                if count == 0:
                    call += "args." + param["name"]
                else:
                    call += ", args." + param["name"]
        
        call += ")" 
        
        return call
        
    def _show_fragment(self, fragment):
        print ''.join(fragment)
        
    def _gen_is_failed_fragment(self):
        if "is_failed" in self.parser.raw_dev_data:
            obj_name = self.parser.device["objects"][0]["name"]
            func_name = self.parser.raw_dev_data["is_failed"]["name"]
            return "    return THIS->" + obj_name + "." + func_name + "();\n"
        else:
            return "    return false;\n"
  
