#pragma once

#include <fstream>
#include <string>
#include <stdexcept>
#include "utils.hpp"

// RAII wrapper for file streams
class FileWrapper {
private:
    std::ifstream m_file;
    std::string m_path;
    bool m_is_open = false;

public:
    explicit FileWrapper(const std::string& path) : m_path(path), m_file(path) {
        m_is_open = m_file.is_open();
        if (!m_is_open) {
            throw std::runtime_error("Could not open file: " + path);
        }
    }
    
    ~FileWrapper() {
        if (m_is_open && m_file.is_open()) {
            m_file.close();
        }
    }
    
    std::ifstream& get() { return m_file; }
    const std::string& path() const { return m_path; }
    bool isOpen() const { return m_is_open && m_file.is_open(); }
    
    // Prevent copying
    FileWrapper(const FileWrapper&) = delete;
    FileWrapper& operator=(const FileWrapper&) = delete;
    
    // Allow moving
    FileWrapper(FileWrapper&& other) noexcept 
        : m_file(std::move(other.m_file)), m_path(std::move(other.m_path)), m_is_open(other.m_is_open) {
        other.m_is_open = false;
    }
    
    FileWrapper& operator=(FileWrapper&& other) noexcept {
        if (this != &other) {
            if (m_is_open && m_file.is_open()) {
                m_file.close();
            }
            m_file = std::move(other.m_file);
            m_path = std::move(other.m_path);
            m_is_open = other.m_is_open;
            other.m_is_open = false;
        }
        return *this;
    }
};

// RAII wrapper for output file streams
class OutputFileWrapper {
private:
    std::ofstream m_file;
    std::string m_path;
    bool m_is_open = false;

public:
    explicit OutputFileWrapper(const std::string& path, std::ios_base::openmode mode = std::ios_base::trunc) 
        : m_path(path), m_file(path, mode) {
        m_is_open = m_file.is_open();
        if (!m_is_open) {
            throw std::runtime_error("Could not open file for writing: " + path);
        }
    }
    
    ~OutputFileWrapper() {
        if (m_is_open && m_file.is_open()) {
            m_file.close();
        }
    }
    
    std::ofstream& get() { return m_file; }
    const std::string& path() const { return m_path; }
    bool isOpen() const { return m_is_open && m_file.is_open(); }
    
    // Prevent copying
    OutputFileWrapper(const OutputFileWrapper&) = delete;
    OutputFileWrapper& operator=(const OutputFileWrapper&) = delete;
    
    // Allow moving
    OutputFileWrapper(OutputFileWrapper&& other) noexcept 
        : m_file(std::move(other.m_file)), m_path(std::move(other.m_path)), m_is_open(other.m_is_open) {
        other.m_is_open = false;
    }
    
    OutputFileWrapper& operator=(OutputFileWrapper&& other) noexcept {
        if (this != &other) {
            if (m_is_open && m_file.is_open()) {
                m_file.close();
            }
            m_file = std::move(other.m_file);
            m_path = std::move(other.m_path);
            m_is_open = other.m_is_open;
            other.m_is_open = false;
        }
        return *this;
    }
};

// RAII wrapper for process pipes
class ProcessPipe {
private:
    FILE* m_pipe = nullptr;
    std::string m_command;
    bool m_is_open = false;

public:
    explicit ProcessPipe(const std::string& command) : m_command(command) {
        m_pipe = popen(command.c_str(), "r");
        m_is_open = (m_pipe != nullptr);
        if (!m_is_open) {
            throw std::runtime_error("Failed to execute command: " + command);
        }
    }
    
    ~ProcessPipe() {
        if (m_is_open && m_pipe) {
            if (pclose(m_pipe) == -1) {
                // Log error but don't throw in destructor
                Utils::log(Utils::ERR, "Failed to close pipe for command: {}", m_command);
            }
            m_pipe = nullptr;
            m_is_open = false;
        }
    }
    
    FILE* get() { return m_pipe; }
    const std::string& command() const { return m_command; }
    bool isOpen() const { return m_is_open && m_pipe != nullptr; }
    
    // Prevent copying
    ProcessPipe(const ProcessPipe&) = delete;
    ProcessPipe& operator=(const ProcessPipe&) = delete;
    
    // Allow moving
    ProcessPipe(ProcessPipe&& other) noexcept 
        : m_pipe(other.m_pipe), m_command(std::move(other.m_command)), m_is_open(other.m_is_open) {
        other.m_pipe = nullptr;
        other.m_is_open = false;
    }
    
    ProcessPipe& operator=(ProcessPipe&& other) noexcept {
        if (this != &other) {
            if (m_is_open && m_pipe) {
                if (pclose(m_pipe) == -1) {
                    Utils::log(Utils::ERR, "Failed to close pipe for command: {}", m_command);
                }
            }
            m_pipe = other.m_pipe;
            m_command = std::move(other.m_command);
            m_is_open = other.m_is_open;
            other.m_pipe = nullptr;
            other.m_is_open = false;
        }
        return *this;
    }
};
